#include "config.h"
#include "create_sphere.h"
#include "Scene_c3t3_item.h"

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QtCore/qglobal.h>

#include <map>
#include <vector>
#include <CGAL/gl.h>
#include <CGAL/Mesh_3/dihedral_angle_3.h>

#include <CGAL/Three/Scene_interface.h>

#include <QGLViewer/manipulatedFrame.h>
#include <QGLViewer/qglviewer.h>

struct Scene_c3t3_item_priv {
  Scene_c3t3_item_priv() : c3t3() {}
  Scene_c3t3_item_priv(const C3t3& c3t3_) : c3t3(c3t3_) {}

  C3t3 c3t3;
  QVector<QColor> colors;
};

void Scene_c3t3_item::compile_shaders()
{
    program_sphere = new QOpenGLShaderProgram();

    //Source code
    const char vertex_source[] =
    {
        "#version 120                                                                                             \n"
        "attribute highp vec4 vertex;                                                                             \n"
        "attribute highp vec3 normals;                                                                            \n"
        "attribute highp vec3 colors;                                                                             \n"
        "attribute highp vec3 center;                                                                             \n"
        "attribute highp float radius;                                                                            \n"
        "uniform highp mat4 mvp_matrix;                                                                           \n"
        "uniform highp mat4 mv_matrix;                                                                            \n"
        "varying highp vec4 fP;                                                                                   \n"
        "varying highp vec3 fN;                                                                                   \n"
        "varying highp vec4 color;                                                                                \n"
        "                                                                                                         \n"
        "                                                                                                         \n"
        "void main(void)                                                                                          \n"
        "{                                                                                                        \n"
        "  color = vec4(colors,1.0);                                                                            \n"
        "  fP = mv_matrix * vertex;                                                                                 \n"
        "  fN = mat3(mv_matrix)* normals;                                                                           \n"
        "   gl_Position =  mvp_matrix *                                                                           \n"
        "  vec4(radius*vertex.x + center.x, radius* vertex.y + center.y, radius*vertex.z + center.z, 1.0) ;                      \n"
        "}                                                                                                        \n"
    };
    QOpenGLShader *vertex_shader = new QOpenGLShader(QOpenGLShader::Vertex);
    if(!vertex_shader->compileSourceCode(vertex_source))
    {
        std::cerr<<"Compiling vertex source FAILED"<<std::endl;
    }


    if(!program_sphere->addShader(vertex_shader))
    {
        std::cerr<<"adding vertex shader FAILED"<<std::endl;
    }
    if(!program_sphere->addShaderFromSourceFile(QOpenGLShader::Fragment,":/cgal/Polyhedron_3/resources/shader_with_light.f" ))
    {
        std::cerr<<"adding fragment shader FAILED"<<std::endl;
    }
    if(!program_sphere->link())
    {
        //std::cerr<<"linking Program FAILED"<<std::endl;
        qDebug() << program_sphere->log();
    }
}
double complex_diag(const Scene_item* item) {
  const Scene_item::Bbox& bbox = item->bbox();
  const double& xdelta = bbox.xmax-bbox.xmin;
  const double& ydelta = bbox.ymax-bbox.ymin;
  const double& zdelta = bbox.zmax-bbox.zmin;
  const double diag = std::sqrt(xdelta*xdelta +
                                ydelta*ydelta +
                                zdelta*zdelta);
  return diag * 0.7;
}

Scene_c3t3_item::Scene_c3t3_item()
  : Scene_item(NumberOfBuffers, NumberOfVaos)
  , d(new Scene_c3t3_item_priv())
  , frame(new ManipulatedFrame())
  , last_known_scene(NULL)
  , data_item_(NULL)
  , histogram_()
  , indices_()
{
  positions_lines.resize(0);
  positions_poly.resize(0);
  normals.resize(0);
  s_vertex.resize(0);
  s_normals.resize(0);
  ws_vertex.resize(0);
  connect(frame, SIGNAL(modified()), this, SLOT(changed()));
  c3t3_changed();
  setRenderingMode(FlatPlusEdges);
  compile_shaders();
  spheres_are_shown = false;
  create_flat_and_wire_sphere(1.0f,s_vertex,s_normals, ws_vertex);
}

Scene_c3t3_item::Scene_c3t3_item(const C3t3& c3t3)
  : Scene_item(NumberOfBuffers, NumberOfVaos)
  , d(new Scene_c3t3_item_priv(c3t3))
  , frame(new ManipulatedFrame())
  , last_known_scene(NULL)  
  , data_item_(NULL)  
  , histogram_()
  , indices_()
{
  positions_lines.resize(0);
  positions_poly.resize(0);
  normals.resize(0);
  s_vertex.resize(0);
  s_normals.resize(0);
  ws_vertex.resize(0);
  connect(frame, SIGNAL(modified()), this, SLOT(changed()));
  c3t3_changed();
  setRenderingMode(FlatPlusEdges);
  compile_shaders();
  spheres_are_shown = false;
  create_flat_and_wire_sphere(1.0f,s_vertex,s_normals, ws_vertex);
}

Scene_c3t3_item::~Scene_c3t3_item()
{
  delete frame;
  delete d;
}


const Scene_item*
Scene_c3t3_item::data_item() const
{
  return data_item_;
}

void
Scene_c3t3_item::set_data_item(const Scene_item* data_item)
{
  data_item_ = data_item;
  if (NULL != data_item)
  {
    connect(data_item, SIGNAL(aboutToBeDestroyed()),
      this, SLOT(data_item_destroyed()));
  }
}

void
Scene_c3t3_item::data_item_destroyed()
{
  set_data_item(NULL);
}

const C3t3&
Scene_c3t3_item::c3t3() const {
  return d->c3t3;
}

C3t3&
Scene_c3t3_item::c3t3()
{
  return d->c3t3;
}

void
Scene_c3t3_item::changed()
{
  this->c3t3_changed();
}

void
Scene_c3t3_item::c3t3_changed()
{
  // Update colors
  // Fill indices map and get max subdomain value
  indices_.clear();

  int max = 0;
  for (C3t3::Cells_in_complex_iterator cit = this->c3t3().cells_in_complex_begin(),
    end = this->c3t3().cells_in_complex_end(); cit != end; ++cit)
  {
    max = (std::max)(max, cit->subdomain_index());
    indices_.insert(cit->subdomain_index());
  }

  d->colors.resize(max + 1);
  compute_color_map(color_);

  // Rebuild histogram
  build_histogram();
  //compute_elements();
  this->invalidate_buffers();
}

QPixmap
Scene_c3t3_item::graphicalToolTip() const
{
  if (!histogram_.isNull())
  {
    return histogram_;
  }
  else
  {
    const_cast<Scene_c3t3_item&>(*this).build_histogram();
    return histogram_;
  }
}

template<typename C3t3>
std::vector<int>
create_histogram(const C3t3& c3t3, double& min_value, double& max_value)
{
  typedef typename C3t3::Triangulation::Point Point_3;

  std::vector<int> histo(181, 0);

  min_value = 180.;
  max_value = 0.;

  for (typename C3t3::Cells_in_complex_iterator cit = c3t3.cells_in_complex_begin();
    cit != c3t3.cells_in_complex_end();
    ++cit)
  {
    if (!c3t3.is_in_complex(cit))
      continue;

#ifdef CGAL_MESH_3_DEMO_DONT_COUNT_TETS_ADJACENT_TO_SHARP_FEATURES_FOR_HISTOGRAM
    if (c3t3.in_dimension(cit->vertex(0)) <= 1
      || c3t3.in_dimension(cit->vertex(1)) <= 1
      || c3t3.in_dimension(cit->vertex(2)) <= 1
      || c3t3.in_dimension(cit->vertex(3)) <= 1)
      continue;
#endif //CGAL_MESH_3_DEMO_DONT_COUNT_TETS_ADJACENT_TO_SHARP_FEATURES_FOR_HISTOGRAM

    const Point_3& p0 = cit->vertex(0)->point();
    const Point_3& p1 = cit->vertex(1)->point();
    const Point_3& p2 = cit->vertex(2)->point();
    const Point_3& p3 = cit->vertex(3)->point();

    double a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p0, p1, p2, p3)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

    a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p0, p2, p1, p3)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

    a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p0, p3, p1, p2)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

    a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p1, p2, p0, p3)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

    a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p1, p3, p0, p2)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

    a = CGAL::to_double(CGAL::abs(CGAL::Mesh_3::dihedral_angle(p2, p3, p0, p1)));
    histo[static_cast<int>(std::floor(a))] += 1;
    min_value = (std::min)(min_value, a);
    max_value = (std::max)(max_value, a);

  }

  return histo;
}

void
Scene_c3t3_item::build_histogram()
{
#ifdef CGAL_MESH_3_DEMO_BIGGER_HISTOGRAM_WITH_WHITE_BACKGROUNG
  // Create an histogram_ and display it
  const int height = 280;
  const int top_margin = 5;
  const int left_margin = 20;
  const int drawing_height = height - top_margin * 2;
  const int width = 804;
  const int cell_width = 4;
  const int text_margin = 3;
  const int text_height = 34;

  histogram_ = QPixmap(width, height + text_height);
  histogram_.fill(QColor(255, 255, 255));
#else
  // Create an histogram_ and display it
  const int height = 140;
  const int top_margin = 5;
  const int left_margin = 20;
  const int drawing_height = height - top_margin * 2;
  const int width = 402;
  const int cell_width = 2;
  const int text_margin = 3;
  const int text_height = 20;

  histogram_ = QPixmap(width, height + text_height);
  histogram_.fill(QColor(192, 192, 192));
#endif  

  QPainter painter(&histogram_);
  painter.setPen(Qt::black);
  painter.setBrush(QColor(128, 128, 128));
  //painter.setFont(QFont("Arial", 30));

  // Build histogram_ data
  double min_value, max_value;
  std::vector<int> histo_data = create_histogram(c3t3(), min_value, max_value);

  // Get maximum value (to normalize)
  int max_size = 0;
  for (std::vector<int>::iterator it = histo_data.begin(), end = histo_data.end();
    it != end; ++it)
  {
    max_size = (std::max)(max_size, *it);
  }

  // colored histogram
  int j = 0;

  // draw
  int i = left_margin;
  for (std::vector<int>::iterator it = histo_data.begin(), end = histo_data.end();
    it != end; ++it, i += cell_width)
  {
    int line_height = static_cast<int>(std::ceil(static_cast<double>(drawing_height)*
      static_cast<double>(*it) / static_cast<double>(max_size)) + .5);

    painter.fillRect(i,
      drawing_height + top_margin - line_height,
      cell_width,
      line_height,
      get_histogram_color(j++));
  }

  // draw bottom horizontal line
  painter.setPen(Qt::blue);

  painter.drawLine(QPoint(left_margin, drawing_height + top_margin),
    QPoint(left_margin + static_cast<int>(histo_data.size())*cell_width,
    drawing_height + top_margin));


  // draw min value and max value
  const int min_tr_width = static_cast<int>(2 * (std::floor(min_value)*cell_width + left_margin));
  const int max_tr_width = static_cast<int>(
    2 * ((histo_data.size() - std::floor(max_value))*cell_width + left_margin));
  const int tr_y = drawing_height + top_margin + text_margin;

  painter.setPen(get_histogram_color(min_value));
  QRect min_text_rect(0, tr_y, min_tr_width, text_height);
  painter.drawText(min_text_rect, Qt::AlignCenter, tr("%1").arg(min_value, 0, 'f', 1));

  painter.setPen(get_histogram_color(max_value));
  QRect max_text_rect(width - max_tr_width, tr_y, max_tr_width, text_height);
  painter.drawText(max_text_rect, Qt::AlignCenter, tr("%1").arg(max_value, 0, 'f', 1));
}

QColor
Scene_c3t3_item::get_histogram_color(const double v) const
{
  if (v < 5)            { return Qt::red; }
  else if (v < 10)      { return QColor(215, 108, 0); }
  else if (v < 15)      { return QColor(138, 139, 0); }
  else if (v < 165)     { return QColor(60, 136, 64); }
  else if (v < 170)     { return QColor(138, 139, 1); }
  else if (v < 175)     { return QColor(215, 108, 0); }
  else /* 175<v<=180 */   { return Qt::red; }
}

void
Scene_c3t3_item::update_histogram()
{
  build_histogram();
}

void
Scene_c3t3_item::compute_color_map(const QColor& c)
{
  typedef Indices::size_type size_type;

  size_type nb_domains = indices_.size();
  size_type i = 0;
  for (Indices::iterator it = indices_.begin(), end = indices_.end();
    it != end; ++it, ++i)
  {
    double hue = c.hueF() + 1. / nb_domains * i;
    if (hue > 1) { hue -= 1.; }
    d->colors[*it] = QColor::fromHsvF(hue, c.saturationF(), c.valueF());
  }
}

Kernel::Plane_3 Scene_c3t3_item::plane() const {
  const qglviewer::Vec& pos = frame->position();
  const qglviewer::Vec& n =
    frame->inverseTransformOf(qglviewer::Vec(0.f, 0.f, 1.f));
  return Kernel::Plane_3(n[0], n[1], n[2], -n * pos);
}

Scene_item::Bbox Scene_c3t3_item::bbox() const {
  if (isEmpty())
    return Bbox();
  else {
    CGAL::Bbox_3 result = c3t3().triangulation().finite_vertices_begin()->point().bbox();
    for (Tr::Finite_vertices_iterator
      vit = ++c3t3().triangulation().finite_vertices_begin(),
      end = c3t3().triangulation().finite_vertices_end();
    vit != end; ++vit)
    {
      result = result + vit->point().bbox();
    }
    return Bbox(result.xmin(), result.ymin(), result.zmin(),
      result.xmax(), result.ymax(), result.zmax());
  }
}

QString Scene_c3t3_item::toolTip() const {
  int number_of_tets = 0;
  for (Tr::Finite_cells_iterator
    cit = c3t3().triangulation().finite_cells_begin(),
    end = c3t3().triangulation().finite_cells_end();
    cit != end; ++cit)
  {
    if (c3t3().is_in_complex(cit))
      ++number_of_tets;
  }
  return tr("<p><b>3D complex in a 3D triangulation</b></p>"
    "<p>Number of vertices: %1<br />"
    "Number of surface facets: %2<br />"
    "Number of volume tetrahedra: %3</p>")
    .arg(c3t3().triangulation().number_of_vertices())
    .arg(c3t3().number_of_facets())
    .arg(number_of_tets);
}

void Scene_c3t3_item::draw(CGAL::Three::Viewer_interface* viewer) const {
  if (!are_buffers_filled)
  {
    compute_elements();
    initialize_buffers(viewer);
  }
  vaos[Facets]->bind();
  program = getShaderProgram(PROGRAM_WITH_LIGHT);
  attrib_buffers(viewer, PROGRAM_WITH_LIGHT);
  program->bind();
  program->setAttributeValue("colors", this->color());
  viewer->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(positions_poly.size() / 3));
  program->release();
  vaos[Facets]->release();

  if(spheres_are_shown)
  {
      vaos[Spheres]->bind();
      program_sphere->bind();
      //ModelViewMatrix used for the transformation of the camera.
      QMatrix4x4 mvp_mat;
      // ModelView Matrix used for the lighting system
      QMatrix4x4 mv_mat;
      GLdouble d_mat[16];
      GLint is_both_sides = 0;
      viewer->camera()->getModelViewProjectionMatrix(d_mat);
      //Convert the GLdoubles matrices in GLfloats
      for (int i=0; i<16; ++i){
          mvp_mat.data()[i] = GLfloat(d_mat[i]);
      }
      viewer->camera()->getModelViewMatrix(d_mat);
      for (int i=0; i<16; ++i)
          mv_mat.data()[i] = GLfloat(d_mat[i]);
      QVector4D position(0.0f,0.0f,1.0f, 1.0f );
      QVector4D ambient(0.4f, 0.4f, 0.4f, 0.4f);
      // Diffuse
      QVector4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
      // Specular
      QVector4D specular(0.0f, 0.0f, 0.0f, 1.0f);
      viewer->glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &is_both_sides);


      program_sphere->setUniformValue("mvp_matrix", mvp_mat);
      program_sphere->setUniformValue("mv_matrix", mv_mat);
      program_sphere->setUniformValue("light_pos", position);
      program_sphere->setUniformValue("light_diff",diffuse);
      program_sphere->setUniformValue("light_spec", specular);
      program_sphere->setUniformValue("light_amb", ambient);
      program_sphere->setUniformValue("spec_power", 51.8f);
      program_sphere->setUniformValue("is_two_side", is_both_sides);

      viewer->glDrawArraysInstanced(GL_TRIANGLES, 0,
                                  static_cast<GLsizei>(s_vertex.size()/3),
                                  static_cast<GLsizei>(s_radius.size()));
      program_sphere->release();
      vaos[Spheres]->release();
  }
}

void Scene_c3t3_item::draw_edges(CGAL::Three::Viewer_interface* viewer) const {
  if (!are_buffers_filled)
  {
    compute_elements();
    initialize_buffers(viewer);
  }
  vaos[2]->bind();
  program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
  attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
  program->bind();
  program->setAttributeValue("colors", QColor(Qt::black));
  QMatrix4x4 f_mat;
  for (int i = 0; i<16; i++)
    f_mat.data()[i] = frame->matrix()[i];
  program->setUniformValue("f_matrix", f_mat);
  viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_grid.size() / 3));
  program->release();
  vaos[2]->release();

  vaos[Edges]->bind();
  program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
  attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
  program->bind();
  program->setAttributeValue("colors", QColor(Qt::black));
  viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_lines.size() / 3));
  program->release();
  vaos[Edges]->release();

  if(spheres_are_shown)
  {
      vaos[Wired_spheres]->bind();
      program_sphere->bind();
      //ModelViewMatrix used for the transformation of the camera.
      QMatrix4x4 mvp_mat;
      // ModelView Matrix used for the lighting system
      QMatrix4x4 mv_mat;
      GLdouble d_mat[16];
      GLint is_both_sides = 0;
      viewer->camera()->getModelViewProjectionMatrix(d_mat);
      //Convert the GLdoubles matrices in GLfloats
      for (int i=0; i<16; ++i){
          mvp_mat.data()[i] = GLfloat(d_mat[i]);
      }
      viewer->camera()->getModelViewMatrix(d_mat);
      for (int i=0; i<16; ++i)
          mv_mat.data()[i] = GLfloat(d_mat[i]);
      QVector4D position(0.0f,0.0f,1.0f, 1.0f );
      QVector4D ambient(0.4f, 0.4f, 0.4f, 0.4f);
      // Diffuse
      QVector4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
      // Specular
      QVector4D specular(0.0f, 0.0f, 0.0f, 1.0f);
      viewer->glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &is_both_sides);


      program_sphere->setUniformValue("mvp_matrix", mvp_mat);
      program_sphere->setUniformValue("mv_matrix", mv_mat);
      program_sphere->setUniformValue("light_pos", position);
      program_sphere->setUniformValue("light_diff",diffuse);
      program_sphere->setUniformValue("light_spec", specular);
      program_sphere->setUniformValue("light_amb", ambient);
      program_sphere->setUniformValue("spec_power", 51.8f);
      program_sphere->setUniformValue("is_two_side", is_both_sides);

      viewer->glDrawArraysInstanced(GL_TRIANGLES, 0,
                                  static_cast<GLsizei>(ws_vertex.size()/3),
                                  static_cast<GLsizei>(s_radius.size()));
      program_sphere->release();
      vaos[Wired_spheres]->release();
  }
}

void Scene_c3t3_item::draw_points(CGAL::Three::Viewer_interface * viewer) const
{
  if (!are_buffers_filled)
  {
    compute_elements();
    initialize_buffers(viewer);
  }
  vaos[Edges]->bind();
  program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
  attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
  program->bind();
  program->setAttributeValue("colors", this->color());
  viewer->glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(positions_lines.size() / 3));
  vaos[Edges]->release();
  program->release();

  vaos[2]->bind();
  program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
  attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
  program->bind();
  program->setAttributeValue("colors", this->color());
  QMatrix4x4 f_mat;
  for (int i = 0; i<16; i++)
    f_mat.data()[i] = frame->matrix()[i];
  program->setUniformValue("f_matrix", f_mat);
  viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_grid.size() / 3));
  program->release();
  vaos[2]->release();
}

void Scene_c3t3_item::draw_triangle(const Kernel::Point_3& pa,
  const Kernel::Point_3& pb,
  const Kernel::Point_3& pc, bool /* is_cut */) const {

  #undef darker
  Kernel::Vector_3 n = cross_product(pb - pa, pc - pa);
  n = n / CGAL::sqrt(n*n);


  for (int i = 0; i<3; i++)
  {
    normals.push_back(n.x());
    normals.push_back(n.y());
    normals.push_back(n.z());
  }
  positions_poly.push_back(pa.x());
  positions_poly.push_back(pa.y());
  positions_poly.push_back(pa.z());

  positions_poly.push_back(pb.x());
  positions_poly.push_back(pb.y());
  positions_poly.push_back(pb.z());

  positions_poly.push_back(pc.x());
  positions_poly.push_back(pc.y());
  positions_poly.push_back(pc.z());

}

void Scene_c3t3_item::draw_triangle_edges(const Kernel::Point_3& pa,
  const Kernel::Point_3& pb,
  const Kernel::Point_3& pc)const {

#undef darker
  Kernel::Vector_3 n = cross_product(pb - pa, pc - pa);
  n = n / CGAL::sqrt(n*n);
  positions_lines.push_back(pa.x());
  positions_lines.push_back(pa.y());
  positions_lines.push_back(pa.z());

  positions_lines.push_back(pb.x());
  positions_lines.push_back(pb.y());
  positions_lines.push_back(pb.z());

  positions_lines.push_back(pb.x());
  positions_lines.push_back(pb.y());
  positions_lines.push_back(pb.z());

  positions_lines.push_back(pc.x());
  positions_lines.push_back(pc.y());
  positions_lines.push_back(pc.z());

  positions_lines.push_back(pc.x());
  positions_lines.push_back(pc.y());
  positions_lines.push_back(pc.z());

  positions_lines.push_back(pa.x());
  positions_lines.push_back(pa.y());
  positions_lines.push_back(pa.z());

}

double Scene_c3t3_item::complex_diag() const {
  const Bbox& bbox = this->bbox();
  const double& xdelta = bbox.xmax - bbox.xmin;
  const double& ydelta = bbox.ymax - bbox.ymin;
  const double& zdelta = bbox.zmax - bbox.zmin;
  const double diag = std::sqrt(xdelta*xdelta +
    ydelta*ydelta +
    zdelta*zdelta);
  return diag * 0.7;
}

void Scene_c3t3_item::export_facets_in_complex()
{
  std::stringstream off_sstream;
  c3t3().output_facets_in_complex_to_off(off_sstream);
  std::string backup = off_sstream.str();
  // Try to read .off in a polyhedron
  Scene_polyhedron_item* item = new Scene_polyhedron_item;
  if (!item->load(off_sstream))
  {
    delete item;
    off_sstream.str(backup);

    // Try to read .off in a polygon soup
    Scene_polygon_soup_item* soup_item = new Scene_polygon_soup_item;

    if (!soup_item->load(off_sstream)) {
      delete soup_item;
      return;
    }

    soup_item->setName(QString("%1_%2").arg(this->name()).arg("facets"));
    last_known_scene->addItem(soup_item);
  }
  else{
    item->setName(QString("%1_%2").arg(this->name()).arg("facets"));
    last_known_scene->addItem(item);
  }
}

QMenu* Scene_c3t3_item::contextMenu()
{
  const char* prop_name = "Menu modified by Scene_c3t3_item.";

  QMenu* menu = Scene_item::contextMenu();

  // Use dynamic properties:
  // http://doc.qt.io/qt-5/qobject.html#property
  bool menuChanged = menu->property(prop_name).toBool();

  if (!menuChanged) {
    QAction* actionExportFacetsInComplex =
      menu->addAction(tr("Export facets in complex"));
    actionExportFacetsInComplex->setObjectName("actionExportFacetsInComplex");
    connect(actionExportFacetsInComplex,
      SIGNAL(triggered()), this,
      SLOT(export_facets_in_complex()));

    QAction* actionShowSpheres =
      menu->addAction(tr("Show protecting &spheres"));
    actionShowSpheres->setCheckable(true);
    actionShowSpheres->setObjectName("actionShowSpheres");
    connect(actionShowSpheres, SIGNAL(toggled(bool)),
            this, SLOT(show_spheres(bool)));
    menu->setProperty(prop_name, true);
  }
  return menu;
}

void Scene_c3t3_item::initialize_buffers(CGAL::Three::Viewer_interface *viewer)const
{
  //vao containing the data for the facets
  {
    program = getShaderProgram(PROGRAM_WITH_LIGHT, viewer);
    program->bind();

    vaos[Facets]->bind();
    buffers[Facet_vertices].bind();
    buffers[Facet_vertices].allocate(positions_poly.data(),
      static_cast<int>(positions_poly.size()*sizeof(float)));
    program->enableAttributeArray("vertex");
    program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
    buffers[Facet_vertices].release();

    buffers[Facet_normals].bind();
    buffers[Facet_normals].allocate(normals.data(),
      static_cast<int>(normals.size()*sizeof(float)));
    program->enableAttributeArray("normals");
    program->setAttributeBuffer("normals", GL_FLOAT, 0, 3);
    buffers[Facet_normals].release();

    vaos[Facets]->release();
    program->release();

  }

  //vao containing the data for the lines
      {
        program = getShaderProgram(PROGRAM_WITHOUT_LIGHT, viewer);
        program->bind();

        vaos[Edges]->bind();
        buffers[Edges_vertices].bind();
        buffers[Edges_vertices].allocate(positions_lines.data(),
          static_cast<int>(positions_lines.size()*sizeof(float)));
        program->enableAttributeArray("vertex");
        program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
        buffers[Edges_vertices].release();

        vaos[Edges]->release();
        program->release();

      }

  //vao containing the data for the grid
      {
        program = getShaderProgram(PROGRAM_WITHOUT_LIGHT, viewer);
        program->bind();

        vaos[2]->bind();
        buffers[Grid_vertices].bind();
        buffers[Grid_vertices].allocate(positions_grid.data(),
          static_cast<int>(positions_grid.size()*sizeof(float)));
        program->enableAttributeArray("vertex");
        program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
        buffers[Grid_vertices].release();
        vaos[2]->release();
        program->release();
      }

  //vao containing the data for the spheres
    {
      program_sphere->bind();

      vaos[Spheres]->bind();
      buffers[Sphere_vertices].bind();
      buffers[Sphere_vertices].allocate(s_vertex.data(),
        static_cast<int>(s_vertex.size()*sizeof(float)));
      program_sphere->enableAttributeArray("vertex");
      program_sphere->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
      buffers[Sphere_vertices].release();

      buffers[Sphere_normals].bind();
      buffers[Sphere_normals].allocate(s_normals.data(),
        static_cast<int>(s_normals.size()*sizeof(float)));
      program_sphere->enableAttributeArray("normals");
      program_sphere->setAttributeBuffer("normals", GL_FLOAT, 0, 3);
      buffers[Sphere_normals].release();

      buffers[Sphere_colors].bind();
      buffers[Sphere_colors].allocate(s_colors.data(),
        static_cast<int>(s_colors.size()*sizeof(float)));
      program_sphere->enableAttributeArray("colors");
      program_sphere->setAttributeBuffer("colors", GL_FLOAT, 0, 3);
      buffers[Sphere_colors].release();

      buffers[Sphere_radius].bind();
      buffers[Sphere_radius].allocate(s_radius.data(),
        static_cast<int>(s_radius.size()*sizeof(float)));
      program_sphere->enableAttributeArray("radius");
      program_sphere->setAttributeBuffer("radius", GL_FLOAT, 0, 3);
      buffers[Sphere_radius].release();

      buffers[Sphere_center].bind();
      buffers[Sphere_center].allocate(s_center.data(),
        static_cast<int>(s_center.size()*sizeof(float)));
      program_sphere->enableAttributeArray("center");
      program_sphere->setAttributeBuffer("center", GL_FLOAT, 0, 3);
      buffers[Sphere_center].release();

      viewer->glVertexAttribDivisor(program_sphere->attributeLocation("center"), 1);
      viewer->glVertexAttribDivisor(program_sphere->attributeLocation("radius"), 1);
      viewer->glVertexAttribDivisor(program_sphere->attributeLocation("colors"), 1);
      vaos[Spheres]->release();

    }

    //vao containing the data for the wired spheres
      {
        program_sphere->bind();

        vaos[Wired_spheres]->bind();
        buffers[Wired_spheres_vertices].bind();
        buffers[Wired_spheres_vertices].allocate(s_vertex.data(),
          static_cast<int>(s_vertex.size()*sizeof(float)));
        program_sphere->enableAttributeArray("vertex");
        program_sphere->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
        buffers[Wired_spheres_vertices].release();

        buffers[Sphere_normals].bind();
        program_sphere->enableAttributeArray("normals");
        program_sphere->setAttributeBuffer("normals", GL_FLOAT, 0, 3);
        buffers[Sphere_normals].release();

        buffers[Sphere_colors].bind();
        program_sphere->enableAttributeArray("colors");
        program_sphere->setAttributeBuffer("colors", GL_FLOAT, 0, 3);
        buffers[Sphere_colors].release();

        buffers[Sphere_radius].bind();
        program_sphere->enableAttributeArray("radius");
        program_sphere->setAttributeBuffer("radius", GL_FLOAT, 0, 3);
        buffers[Sphere_radius].release();

        buffers[Sphere_center].bind();
        program_sphere->enableAttributeArray("center");
        program_sphere->setAttributeBuffer("center", GL_FLOAT, 0, 3);
        buffers[Sphere_center].release();

        viewer->glVertexAttribDivisor(program_sphere->attributeLocation("center"), 1);
        viewer->glVertexAttribDivisor(program_sphere->attributeLocation("radius"), 1);
        viewer->glVertexAttribDivisor(program_sphere->attributeLocation("colors"), 1);
        vaos[Wired_spheres]->release();

        program_sphere->release();
      }
    program_sphere->release();
      are_buffers_filled = true;
}


void Scene_c3t3_item::compute_elements() const
{
  positions_lines.clear();
  positions_poly.clear();
  normals.clear();
  s_colors.resize(0);
  s_center.resize(0);
  s_radius.resize(0);


  //The grid
  {
    float x = (2 * (float)complex_diag()) / 10.0;
    float y = (2 * (float)complex_diag()) / 10.0;
    for (int u = 0; u < 11; u++)
    {

      positions_grid.push_back(-(float)complex_diag() + x* u);
      positions_grid.push_back(-(float)complex_diag());
      positions_grid.push_back(0.0);

      positions_grid.push_back(-(float)complex_diag() + x* u);
      positions_grid.push_back((float)complex_diag());
      positions_grid.push_back(0.0);
    }
    for (int v = 0; v<11; v++)
    {

      positions_grid.push_back(-(float)complex_diag());
      positions_grid.push_back(-(float)complex_diag() + v * y);
      positions_grid.push_back(0.0);

      positions_grid.push_back((float)complex_diag());
      positions_grid.push_back(-(float)complex_diag() + v * y);
      positions_grid.push_back(0.0);
    }
  }


  if (isEmpty())
    return;

  const Kernel::Plane_3& plane = this->plane();
  GLdouble clip_plane[4];
  clip_plane[0] = -plane.a();
  clip_plane[1] = -plane.b();
  clip_plane[2] = -plane.c();
  clip_plane[3] = -plane.d();


  //The facets
  {
    for (C3t3::Facet_iterator
      fit = c3t3().facets_begin(),
      end = c3t3().facets_end();
    fit != end; ++fit)
    {
      const Tr::Cell_handle& cell = fit->first;
      const int& index = fit->second;
      const Kernel::Point_3& pa = cell->vertex((index + 1) & 3)->point();
      const Kernel::Point_3& pb = cell->vertex((index + 2) & 3)->point();
      const Kernel::Point_3& pc = cell->vertex((index + 3) & 3)->point();
      typedef Kernel::Oriented_side Side;
      using CGAL::ON_ORIENTED_BOUNDARY;
      const Side sa = plane.oriented_side(pa);
      const Side sb = plane.oriented_side(pb);
      const Side sc = plane.oriented_side(pc);
      bool is_shown = false;
      if (pa.x() * clip_plane[0] + pa.y() * clip_plane[1] + pa.z() * clip_plane[2] + clip_plane[3]  > 0
        && pb.x() * clip_plane[0] + pb.y() * clip_plane[1] + pb.z() * clip_plane[2] + clip_plane[3]  > 0
        && pc.x() * clip_plane[0] + pc.y() * clip_plane[1] + pc.z() * clip_plane[2] + clip_plane[3]  > 0)
        is_shown = true;

      if (is_shown && sa != ON_ORIENTED_BOUNDARY &&
        sb != ON_ORIENTED_BOUNDARY &&
        sc != ON_ORIENTED_BOUNDARY &&
        sb == sa && sc == sa)
      {
        if ((index % 2 == 1) == c3t3().is_in_complex(cell)) draw_triangle(pb, pa, pc, false);
        else draw_triangle(pa, pb, pc, false);
        draw_triangle_edges(pa, pb, pc);
      }

    }


    for (Tr::Finite_cells_iterator
      cit = c3t3().triangulation().finite_cells_begin(),
      end = c3t3().triangulation().finite_cells_end();
    cit != end; ++cit)
    {
      if (!c3t3().is_in_complex(cit))
        continue;

      const Kernel::Point_3& pa = cit->vertex(0)->point();
      const Kernel::Point_3& pb = cit->vertex(1)->point();
      const Kernel::Point_3& pc = cit->vertex(2)->point();
      const Kernel::Point_3& pd = cit->vertex(3)->point();
      typedef Kernel::Oriented_side Side;
      using CGAL::ON_ORIENTED_BOUNDARY;
      const Side sa = plane.oriented_side(pa);
      const Side sb = plane.oriented_side(pb);
      const Side sc = plane.oriented_side(pc);
      const Side sd = plane.oriented_side(pd);

      if (sa == ON_ORIENTED_BOUNDARY ||
        sb == ON_ORIENTED_BOUNDARY ||
        sc == ON_ORIENTED_BOUNDARY ||
        sd == ON_ORIENTED_BOUNDARY ||
        sb != sa || sc != sa || sd != sa)
      {
        draw_triangle(pb, pa, pc, true);
        draw_triangle(pa, pb, pd, true);
        draw_triangle(pa, pd, pc, true);
        draw_triangle(pb, pc, pd, true);

        draw_triangle_edges(pa, pb, pc);
        draw_triangle_edges(pa, pb, pd);
        draw_triangle_edges(pa, pc, pd);
        draw_triangle_edges(pb, pc, pd);
      }
    }
  }
  //The Spheres
  {

      for(Tr::Finite_vertices_iterator
          vit = d->c3t3.triangulation().finite_vertices_begin(),
          end =  d->c3t3.triangulation().finite_vertices_end();
          vit != end; ++vit)
      {
          typedef Tr::Vertex_handle Vertex_handle;
          std::vector<Vertex_handle> incident_vertices;
          d->c3t3.triangulation().incident_vertices(vit, std::back_inserter(incident_vertices));
          bool red = vit->is_special();
          for(std::vector<Vertex_handle>::const_iterator
              vvit = incident_vertices.begin(), end = incident_vertices.end();
              vvit != end; ++vvit)
          {
              if(Kernel::Sphere_3(vit->point().point(),
                                  vit->point().weight()).bounded_side((*vvit)->point().point())
                      == CGAL::ON_BOUNDED_SIDE)
                  red = true;
          }
          if(red){
              s_colors.push_back(1.0);
              s_colors.push_back(0.0);
              s_colors.push_back(0.0);

          }
          else{
                  QColor c = this->color().darker(250);
                  s_colors.push_back(c.redF());
                  s_colors.push_back(c.greenF());
                  s_colors.push_back(c.blueF());
              }
              s_center.push_back(vit->point().point().x());
              s_center.push_back(vit->point().point().y());
              s_center.push_back(vit->point().point().z());

              s_radius.push_back(CGAL::sqrt(vit->point().weight()));
      }
  }
}



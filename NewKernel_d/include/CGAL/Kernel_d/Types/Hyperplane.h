#ifndef CGAL_KD_TYPE_HYPERPLANE_H
#define CGAL_KD_TYPE_HYPERPLANE_H
#include <CGAL/store_kernel.h>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
namespace CGAL {
template <class R_> class Hyperplane {
	typedef typename Get_type<R_, FT_tag>::type FT_;
	typedef typename Get_type<R_, Vector_tag>::type	Vector_;
	Vector_ v_;
	FT_ s_;

	public:
	Hyperplane(Vector_ const&v, FT_ const&s): v_(v), s_(s) {}
	// TODO: Add a piecewise constructor?

	Vector_ orthogonal_vector()const{return v_;}
	FT_ translation()const{return s_;}
};
namespace CartesianDKernelFunctors {
template <class R_> struct Construct_hyperplane : Store_kernel<R_> {
  CGAL_FUNCTOR_INIT_STORE(Construct_hyperplane)
  typedef typename Get_type<R_, Hyperplane_tag>::type	result_type;
  typedef typename Get_type<R_, Vector_tag>::type	Vector;
  typedef typename Get_type<R_, FT_tag>::type FT;
  typedef typename R_::LA::Square_matrix Matrix;
  private:
  struct One {
    typedef int result_type;
    template<class T>int const& operator()(T const&)const{
      static const int one = 1;
      return one;
    }
  };
  public:

  result_type operator()(Vector const&a, FT const&b)const{
    return result_type(a,b);
  }
  template <class Iter>
  result_type operator()(Iter f, Iter e)const{
    typedef typename R_::LA LA;
    typedef typename LA::Vector Vec;
    typedef typename LA::Construct_vector CVec;
    typedef typename Get_type<R_, Point_tag>::type	Point;
    typename Get_functor<R_, Compute_point_cartesian_coordinate_tag>::type c(this->kernel());
    typename Get_functor<R_, Construct_ttag<Vector_tag> >::type cv(this->kernel());
    typename Get_functor<R_, Point_dimension_tag>::type pd(this->kernel());

    Point const& p0=*f;
    int d = pd(p0);
    Matrix m(d,d);
    for(int j=0;j<d;++j)
      m(0,j)=c(p0,j);
    // Write the point coordinates in lines.
    int i;
    for (i=1; ++f!=e; ++i) {
      Point const& p=*f;
      for(int j=0;j<d;++j)
	m(i,j)=c(p,j);
    }
    CGAL_assertion (i == d);
    Vec one = typename CVec::Iterator()(d,
	boost::make_transform_iterator(boost::counting_iterator<int>(0),One()),
	boost::make_transform_iterator(boost::counting_iterator<int>(d),One()));
    Vec res = typename CVec::Dimension()(d);
    LA::solve(res, CGAL_MOVE(m), CGAL_MOVE(one));
    return this->operator()(cv(d,LA::vector_begin(res),LA::vector_end(res)),1);
  }
};
template <class R_> struct Orthogonal_vector {
  CGAL_FUNCTOR_INIT_IGNORE(Orthogonal_vector)
  typedef typename Get_type<R_, Hyperplane_tag>::type	Hyperplane;
  typedef typename Get_type<R_, Vector_tag>::type	result_type;
  result_type operator()(Hyperplane const&s)const{
    return s.orthogonal_vector();
  }
};
template <class R_> struct Hyperplane_translation {
  CGAL_FUNCTOR_INIT_IGNORE(Hyperplane_translation)
  typedef typename Get_type<R_, Hyperplane_tag>::type	Hyperplane;
  typedef typename Get_type<R_, FT_tag>::type result_type;
  // TODO: Is_exact?
  result_type operator()(Hyperplane const&s)const{
    return s.translation();
  }
};
template <class R_> struct Value_at : Store_kernel<R_> {
  CGAL_FUNCTOR_INIT_STORE(Value_at)
  typedef typename Get_type<R_, Hyperplane_tag>::type	Hyperplane;
  typedef typename Get_type<R_, Vector_tag>::type	Vector;
  typedef typename Get_type<R_, Point_tag>::type	Point;
  typedef typename Get_type<R_, FT_tag>::type		FT;
  typedef FT result_type;
  typedef typename Get_functor<R_, Scalar_product_tag>::type	Dot;
  typedef typename Get_functor<R_, Point_to_vector_tag>::type	P2V;
  result_type operator()(Hyperplane const&h, Point const&p)const{
    Dot dot(this->kernel());
    P2V p2v(this->kernel());
    return dot(h.orthogonal_vector(),p2v(p));
    // Use Orthogonal_vector to make it generic?
    // Copy the code from Scalar_product to avoid p2v?
  }
};
}
//TODO: Add a condition that the hyperplane type is the one from this file.
CGAL_KD_DEFAULT_TYPE(Hyperplane_tag,(CGAL::Hyperplane<K>),(Vector_tag),());
CGAL_KD_DEFAULT_FUNCTOR(Construct_ttag<Hyperplane_tag>,(CartesianDKernelFunctors::Construct_hyperplane<K>),(Vector_tag,Hyperplane_tag),());
CGAL_KD_DEFAULT_FUNCTOR(Orthogonal_vector_tag,(CartesianDKernelFunctors::Orthogonal_vector<K>),(Vector_tag,Hyperplane_tag),());
CGAL_KD_DEFAULT_FUNCTOR(Hyperplane_translation_tag,(CartesianDKernelFunctors::Hyperplane_translation<K>),(Hyperplane_tag),());
CGAL_KD_DEFAULT_FUNCTOR(Value_at_tag,(CartesianDKernelFunctors::Value_at<K>),(Point_tag,Vector_tag,Hyperplane_tag),(Scalar_product_tag,Point_to_vector_tag));
} // namespace CGAL
#endif

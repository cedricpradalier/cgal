// Copyright (c) 2005-2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Partially supported by the IST Programme of the EU as a Shared-cost
// RTD (FET Open) Project under Contract No  IST-2000-26473 
// (ECG - Effective Computational Geometry for Curves and Surfaces) 
// and a STREP (FET Open) Project under Contract No  IST-006413 
// (ACS -- Algorithms for Complex Shapes)
//
// $URL$
// $Id$
//
// Author(s) : Monique Teillaud <Monique.Teillaud@sophia.inria.fr>
//             Sylvain Pion     <Sylvain.Pion@sophia.inria.fr>
//             Pedro Machado    <tashimir@gmail.com>

#ifndef CGAL_SPHERICAL_KERNEL_PREDICATES_HAS_ON_3_H
#define CGAL_SPHERICAL_KERNEL_PREDICATES_HAS_ON_3_H

namespace CGAL {
  namespace SphericalFunctors {

    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_3 &a, 
           const typename SK::Point_3 &p)
    { 
      //~ return a.rep().has_on_boundary(p);
      return a.has_on_boundary(p);
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_with_radius_3 &a, 
           const typename SK::Point_3 &p)
    { 
      //~ return a.rep().has_on_boundary(p);
      return a.has_on_boundary(p);
    }   
    
    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_3 &a, 
           const typename SK::Circular_arc_point_3 &p)
    { 
      typedef typename SK::Algebraic_kernel Algebraic_kernel;
      typedef typename SK::Polynomial_for_spheres_2_3 Equation;
      Equation equation = get_equation<SK>(a);
      return (Algebraic_kernel().sign_at_object()(equation,p.rep().coordinates()) == ZERO);
    }
    
    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_with_radius_3 &a, 
           const typename SK::Circular_arc_point_on_reference_sphere_3 &p){
      return has_on<SK>(static_cast<const typename SK::Sphere_3&>(a),static_cast<const typename SK::Circular_arc_point_3&>(p));
    }
/*
    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &a, 
           const typename SK::Point_3 &p)
    { 
      //~ return a.rep().has_on(p);
      return a.has_on(p);
    }
*/
    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &a, 
           const typename SK::Circular_arc_point_3 &p)
    { 
      typedef typename SK::Algebraic_kernel Algebraic_kernel;
      typedef typename SK::Polynomial_1_3 Equation;
      Equation equation = get_equation<SK>(a);
      return (Algebraic_kernel().sign_at_object()(equation,p.rep().coordinates()) == ZERO);
    }
/*
    template <class SK>
    inline
    bool
    has_on(const typename SK::Line_3 &a,
           const typename SK::Point_3 &p)
    { 
      //~ return a.rep().has_on(p);
      return a.has_on(p);
    }
*/
    template <class SK>
    inline
    bool
    has_on(const typename SK::Line_3 &a, 
           const typename SK::Circular_arc_point_3 &p)
    { 
      typedef typename SK::Algebraic_kernel Algebraic_kernel;
      typedef typename SK::Polynomials_for_line_3 Equation;
      Equation equation = get_equation<SK>(a);
      return p.rep().coordinates().is_on_line(equation);
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circle_3 &a, 
           const typename SK::Point_3 &p)
    { 
      return has_on<SK>(a.diametral_sphere(),p) &&
             has_on<SK>(a.supporting_plane(),p);
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circle_3 &a, 
           const typename SK::Circular_arc_point_3 &p)
    { 
      return has_on<SK>(a.diametral_sphere(),p) &&
             has_on<SK>(a.supporting_plane(),p);
    }

    //duplicated code
    template <class SK>
    inline
    bool
    has_on(const typename SK::Circle_on_reference_sphere_3 &a, 
           const typename SK::Point_3 &p)
    { 
      return has_on<SK>(a.diametral_sphere(),p) &&
             has_on<SK>(a.supporting_plane(),p);
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circle_on_reference_sphere_3 &a, 
           const typename SK::Circular_arc_point_on_reference_sphere_3 &p)
    { 
      return has_on<SK>(a.diametral_sphere(),p) &&
             has_on<SK>(a.supporting_plane(),p);
    }    
    
    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_3 &a, 
           const typename SK::Circle_3 &p)
    { 
      typedef typename SK::Point_3 Point_3;
      typedef typename SK::FT FT;
      Point_3 proj = p.supporting_plane().projection(a.center());
      if(!(proj == p.center())) return false;
      const FT d2 = CGAL::square(a.center().x() - p.center().x()) +
                    CGAL::square(a.center().y() - p.center().y()) +
                    CGAL::square(a.center().z() - p.center().z());
      return ((a.squared_radius() - d2) == p.squared_radius());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &a, 
           const typename SK::Line_3 &p)
    { 
      //~ return a.rep().has_on(p);
      return a.has_on(p);
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &a, 
           const typename SK::Circle_3 &p)
    { 
      return non_oriented_equal<SK>(a,p.supporting_plane());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Line_arc_3 &l, 
           const typename SK::Circular_arc_point_3 &p,
           const bool has_on_supporting_line = false)
    { 
      if(!has_on_supporting_line) {
        if(!has_on<SK>(l.supporting_line(), p)) {
          return false;
        }
      }
      return SK().compare_xyz_3_object()(l.lower_xyz_extremity(),p) <= ZERO &&
             SK().compare_xyz_3_object()(p,l.higher_xyz_extremity()) <= ZERO;
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Line_arc_3 &l, 
           const typename SK::Point_3 &p,
           const bool has_on_supporting_line = false)
    { 
      if(!has_on_supporting_line) {
        if(!has_on<SK>(l.supporting_line(), p)) {
          return false;
        }
      }           
      return SK().compare_xyz_3_object()(l.lower_xyz_extremity(),p) <= ZERO &&
             SK().compare_xyz_3_object()(p,l.higher_xyz_extremity()) <= ZERO;
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &p,
           const typename SK::Line_arc_3 &l)
    { 
      return SK().has_on_3_object()(p, l.supporting_line());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Line_3 &l,
           const typename SK::Line_arc_3 &la)
    { 
      return non_oriented_equal<SK>(l, la.supporting_line());
    }


    template< class SK>
    inline
    Sign
    compute_sign_of_cross_product(const typename SK::Root_of_2 &x1, 
                                  const typename SK::Root_of_2 &y1,
                                  const typename SK::Root_of_2 &z1,
                                  const typename SK::Root_of_2 &x2, 
                                  const typename SK::Root_of_2 &y2,
                                  const typename SK::Root_of_2 &z2) {
      typedef typename SK::Root_of_2 Root_of_2;
      const Root_of_2 cx = y1 * z2 - z1 * y2;
      const Root_of_2 cy = z1 * x2 - x1 * z2;
      const Root_of_2 cz = x1 * y2 - y1 * x2;
      if(!is_zero(cx)) return sign(cx);
      if(!is_zero(cy)) return sign(cy);
      return sign(cz);
    }

    template< class SK>
    inline
    Sign
    compute_sign_of_cross_product(const typename SK::FT &x1, 
                                  const typename SK::FT &y1,
                                  const typename SK::FT &z1,
                                  const typename SK::FT &x2, 
                                  const typename SK::FT &y2,
                                  const typename SK::FT &z2) {
      typedef typename SK::FT FT;
      const FT cx = y1 * z2 - z1 * y2;
      const FT cy = z1 * x2 - x1 * z2;
      const FT cz = x1 * y2 - y1 * x2;
      if(!is_zero(cx)) return sign(cx);
      if(!is_zero(cy)) return sign(cy);
      return sign(cz);
    }

    template< class SK>
    inline
    Sign
    compute_sign_of_cross_product(const typename SK::Circular_arc_point_3 &p1, 
                                  const typename SK::Circular_arc_point_3 &p2,
                                  const typename SK::Point_3 &c) {
      return compute_sign_of_cross_product<SK>(p1.x()-c.x(),
                                               p1.y()-c.y(),
                                               p1.z()-c.z(),
                                               p2.x()-c.x(),
                                               p2.y()-c.y(),
                                               p2.z()-c.z());
    }

    template< class SK>
    inline
    Sign
    compute_sign_of_cross_product(const typename SK::Point_3 &p1, 
                                  const typename SK::Point_3 &p2,
                                  const typename SK::Point_3 &c) {
      return compute_sign_of_cross_product<SK>(p1.x()-c.x(),
                                               p1.y()-c.y(),
                                               p1.z()-c.z(),
                                               p2.x()-c.x(),
                                               p2.y()-c.y(),
                                               p2.z()-c.z());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circular_arc_3 &a, 
           const typename SK::Point_3 &p,
           const bool has_on_supporting_circle = false)
    { 
      if(!has_on_supporting_circle) {
        if(!has_on<SK>(a.supporting_circle(),p)) 
          return false;
      }
      if(a.rep().is_full()) return true;
      const Sign s_x_t = a.sign_cross_product();
      const Sign s_x_p = 
        compute_sign_of_cross_product<SK>(a.source(),p,a.center());
      const Sign p_x_t = 
        compute_sign_of_cross_product<SK>(p,a.target(),a.center());
      if(s_x_t == ZERO) return (s_x_p != NEGATIVE);
      if(a.source() == p) return true;
      if(p == a.target()) return true;
      if(s_x_t == POSITIVE) {
        if(s_x_p == POSITIVE) return p_x_t == POSITIVE;
        else return false;
      } else {
        if(s_x_p == NEGATIVE) return p_x_t == POSITIVE;
        else return true;
      }
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circular_arc_3 &a, 
           const typename SK::Circular_arc_point_3 &p,
           const bool has_on_supporting_circle = false)
    {
      if(!has_on_supporting_circle) {
        if(!has_on<SK>(a.supporting_circle(),p)) 
          return false;
      }
      if(a.rep().is_full()) return true;
      const Sign s_x_t = a.sign_cross_product();
      const Sign s_x_p = 
        compute_sign_of_cross_product<SK>(a.source(),p,a.center());
      const Sign p_x_t = 
        compute_sign_of_cross_product<SK>(p,a.target(),a.center());
      if(s_x_t == ZERO) return (s_x_p != NEGATIVE);
      if(a.source() == p) return true;
      if(p == a.target()) return true;
      if(s_x_t == POSITIVE) {
        if(s_x_p == POSITIVE) return p_x_t == POSITIVE;
        else return false;
      } else {
        if(s_x_p == NEGATIVE) return p_x_t == POSITIVE;
        else return true;
      }
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Sphere_3 &a, 
           const typename SK::Circular_arc_3 &p)
    { 
      return has_on<SK>(a,p.supporting_circle());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Plane_3 &a, 
           const typename SK::Circular_arc_3 &p)
    { 
      return has_on<SK>(a,p.supporting_circle());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circle_3 &c, 
           const typename SK::Circular_arc_3 &ca)
    { 
      return non_oriented_equal<SK>(c,ca.supporting_circle());
    }

    template <class SK>
    inline
    bool
    has_on(const typename SK::Circular_arc_3 &ca,
           const typename SK::Circle_3 &c)
    { 
       if(!non_oriented_equal<SK>(c,ca.supporting_circle())) return false;
       return ca.rep().is_full();
    }

  }//SphericalFunctors
}//CGAL

#endif //CGAL_SPHERICAL_KERNEL_PREDICATES_HAS_ON_3_H

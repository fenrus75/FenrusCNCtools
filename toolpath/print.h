/* from the CGAL examples */

#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>

extern "C" {
#include "toolpath.h"
}

extern const char * color;
template<class K>
void print_point ( CGAL::Point_2<K> const& p )
{
  std::cout << "(" << p.x() << "," << p.y() << ")" ;
}

template<class K, class C>
void print_polygon ( CGAL::Polygon_2<K,C> const& poly, const char * color )
{
  typedef CGAL::Polygon_2<K,C> Polygon ;
  
//  std::cout << "Polygon with " << poly.size() << " vertices" << std::endl ;
  
  for( typename Polygon::Vertex_const_iterator vi = poly.vertices_begin() ; vi != poly.vertices_end() ; ++ vi )
  {
    typename Polygon::Vertex_const_iterator vi2;
    vi2 = vi+1;
    if (vi2 == poly.vertices_end())
      vi2 = poly.vertices_begin();
      
    svg_line(vi->x(), vi->y(), vi2->x(), vi2->y(), color, 0.42);
    
  }
}

template<class K, class C>
void print_polygon ( CGAL::Polygon_2<K,C> const * poly, const char * color )
{
  typedef CGAL::Polygon_2<K,C> Polygon ;
  
//  std::cout << "Polygon with " << poly->size() << " vertices and area " << poly->area() << std::endl ;
  
  for( typename Polygon::Vertex_const_iterator vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi )
  {
    typename Polygon::Vertex_const_iterator vi2;
    vi2 = vi+1;
    if (vi2 == poly->vertices_end())
      vi2 = poly->vertices_begin();
      
    svg_line(vi->x(), vi->y(), vi2->x(), vi2->y(), color, 0.42);
    
  }
}

template<class K, class C>
void print_polygon ( CGAL::Polygon_2<K,C> const * poly, const char * color, double width )
{
  typedef CGAL::Polygon_2<K,C> Polygon ;
  
//  std::cout << "Polygon with " << poly->size() << " vertices and area " << poly->area() << std::endl ;
  
  for( typename Polygon::Vertex_const_iterator vi = poly->vertices_begin() ; vi != poly->vertices_end() ; ++ vi )
  {
    typename Polygon::Vertex_const_iterator vi2;
    vi2 = vi+1;
    if (vi2 == poly->vertices_end())
      vi2 = poly->vertices_begin();
      
    svg_line(vi->x(), vi->y(), vi2->x(), vi2->y(), color, width);
    
  }
}


template<class K, class C>
void print_polygons ( std::vector< boost::shared_ptr< CGAL::Polygon_2<K,C> > > const& polies, const char *color )
{
  typedef std::vector< boost::shared_ptr< CGAL::Polygon_2<K,C> > > PolygonVector ;
  
  for( typename PolygonVector::const_iterator pi = polies.begin() ; pi != polies.end() ; ++ pi )
    print_polygon(**pi, color);
}

template<class K, class C>
void print_polygon_with_holes ( CGAL::Polygon_with_holes_2<K,C> const& polywh, const char *color, const char *holecolor )
{
  typedef CGAL::Polygon_with_holes_2<K,C> PolygonWithHoles ;
  
  std::cout << "Polygon_with_holes having " << polywh.number_of_holes() << " holes" << std::endl ;
  
  print_polygon(polywh.outer_boundary(), color);
  
  for( typename PolygonWithHoles::Hole_const_iterator hi = polywh.holes_begin() ; hi != polywh.holes_end() ; ++ hi )
    print_polygon(*hi, holecolor);
    
}

template<class K, class C>
void print_polygon_with_holes ( CGAL::Polygon_with_holes_2<K,C> const * polywh, const char *color, const char *holecolor )
{
  typedef CGAL::Polygon_with_holes_2<K,C> PolygonWithHoles ;
  
  std::cout << "Polygon_with_holes having " << polywh->number_of_holes() << " holes" << std::endl ;
  
  print_polygon(polywh->outer_boundary(), color);
  
  for( typename PolygonWithHoles::Hole_const_iterator hi = polywh->holes_begin() ; hi != polywh->holes_end() ; ++ hi )
    print_polygon(*hi, holecolor);
    
}

template<class K, class C>
void print_polygons_with_holes ( std::vector< boost::shared_ptr< CGAL::Polygon_with_holes_2<K,C> > > const& polies, const char *color, const char *holecolor )
{

  typedef std::vector< boost::shared_ptr< CGAL::Polygon_with_holes_2<K,C> > > PolygonWithHolesVector ;

  std::cout << "Polygon_with_holes list with " << polies.size() << " element" << std::endl ;

  for( typename PolygonWithHolesVector::const_iterator pi = polies.begin() ; pi != polies.end() ; ++ pi )
    print_polygon_with_holes(**pi, color, holecolor);
}


template<class K>
void print_straight_skeleton( CGAL::Straight_skeleton_2<K> const& ss )
{
  typedef CGAL::Straight_skeleton_2<K> Ss ;
  
  typedef typename Ss::Vertex_const_handle     Vertex_const_handle ;
  typedef typename Ss::Halfedge_const_handle   Halfedge_const_handle ;
  typedef typename Ss::Halfedge_const_iterator Halfedge_const_iterator ;
  
  Halfedge_const_handle null_halfedge ;
  Vertex_const_handle   null_vertex ;

  std::cout << "Straight skeleton with " << ss.size_of_vertices() 
            << " vertices, " << ss.size_of_halfedges()
            << " halfedges and " << ss.size_of_faces()
            << " faces" << std::endl ;
            
  for ( Halfedge_const_iterator i = ss.halfedges_begin(); i != ss.halfedges_end(); ++i )
  {
    const char *color;
    double width = 0.0;
//    print_point(i->opposite()->vertex()->point()) ;
//    std::cout << "->" ;
//    print_point(i->vertex()->point());
    color = "orange";
    if (i->is_inner_bisector()) {
      color = "purple";
      width = 0.15;
    }
    std::cout << " " << ( i->is_bisector() ? "bisector" : "contour" ) << color << std::endl;
    
    if (i->is_bisector())
    svg_line(i->opposite()->vertex()->point().x(), i->opposite()->vertex()->point().y(), 
       i->vertex()->point().x(), i->vertex()->point().y(), color, width);
  }
}

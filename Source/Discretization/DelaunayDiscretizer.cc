#include "Discretization/DelaunayDiscretizer.hh"

#include "Discretization/PolygonDiscretizer.hh"
#include "Shape/PointUtilities.hh"
#include "Shape/TriangleUtilities.hh"

#include <cassert>
#include <exception>
#include <limits>

namespace
{
  typedef Delaunay::Shape::Point Vector;
  typedef Delaunay::Mesh::Vertex Vertex;

  const double EPS = 1.e-6;

  int ConvexityOfStep(const Vertex* vtx0,const Vertex* vtx1,const Vertex* vtx2)
  {
    // -1: the step breaks convexity
    // 0:  the step is linear
    // 1:  the step maintains convexity

    Vector v1 = (*vtx1) - (*vtx0);
    Vector v2 = (*vtx2) - (*vtx1);

    double unnormalizedSin = v1.x*v2.y - v2.x*v1.y;
    if (fabs(unnormalizedSin) < EPS)
      return 0;
    else if (unnormalizedSin > 0.)
      return 1;
    else
      return -1;
  }
}

namespace Delaunay
{
namespace Discretization
{

void DelaunayDiscretizer::Mesh(const Delaunay::Shape::Polygon& polygon,
			       Delaunay::Mesh::Mesh& mesh)
{
  Shape::PointVector vec;
  for (Shape::PointVector::const_iterator it=polygon.GetPoints().begin();
       it!= polygon.GetPoints().end();++it)
  {
    const Mesh::Vertex& vtx = *(this->GetVertices(mesh).emplace(*it)).first;
    vec.push_back(std::cref(static_cast<const Shape::Point&>(vtx)));
  }

  this->GetPerimeter(mesh).SetPoints(vec);
}

void DelaunayDiscretizer::AddPerimeterPoint(const Point& p,
					    Delaunay::Mesh::Mesh& mesh)
{
  Shape::PointVector vec(this->GetPerimeter(mesh).GetPoints());

  const Mesh::Vertex& vtx = *(this->GetVertices(mesh).emplace(p)).first;

  this->GetPerimeter(mesh).SetPoints(vec);
  this->GetVertices(mesh).insert(vtx);

  if (this->GetVertices(mesh).size()<3)
    return;

  if (this->GetVertices(mesh).size()==3)
  {
    const Mesh::Vertex* vs[3];
    Mesh::Mesh::VertexSet::iterator it = this->GetVertices(mesh).begin();
    for (unsigned i=0;i<3;i++)
      vs[i] = &(*it++);

    const Mesh::Edge& ab =*(this->GetEdges(mesh).emplace(*vs[0],*vs[1])).first;
    const Mesh::Edge& bc =*(this->GetEdges(mesh).emplace(*vs[1],*vs[2])).first;
    const Mesh::Edge& ac =*(this->GetEdges(mesh).emplace(*vs[0],*vs[2])).first;
    this->GetTriangles(mesh).emplace(ab,bc,ac);

    return;
  }
  ExtendMesh(&vtx, mesh);
}

void DelaunayDiscretizer::ConstructInitialMeshFromPerimeter(
  Delaunay::Mesh::Mesh& mesh)
{
  if (GetTriangles(mesh).size() != 0)
    return;

  if (this->GetPerimeter(mesh).GetPoints().size() < 3)
    throw(std::domain_error("Too few perimeter elements"));

  PolygonDiscretizer polygonDiscretizer;
  polygonDiscretizer.Mesh(this->GetPerimeter(mesh), mesh);
}

void DelaunayDiscretizer::AddInteriorPoint(const Point& p,
					   Delaunay::Mesh::Mesh& mesh)
{
  if (this->GetTriangles(mesh).size() == 0)
    ConstructInitialMeshFromPerimeter(mesh);

  const Mesh::Vertex& vtx = *(this->GetVertices(mesh).emplace(p)).first;

  const Mesh::Triangle* containingTriangle = FindContainingTriangle(vtx,mesh);

  if (containingTriangle)
  {
    SplitTriangle(containingTriangle, &vtx, mesh);
  }
}

const Mesh::Triangle* DelaunayDiscretizer::FindContainingTriangle(
  const Point& p, Delaunay::Mesh::Mesh& mesh) const
{
  for (auto it = mesh.GetTriangles().begin();
       it != mesh.GetTriangles().end(); ++it)
    if (Contains(*it, p))
      return &(*it);

  return nullptr;
}

void DelaunayDiscretizer::SplitTriangle(const Mesh::Triangle* t,
					const Mesh::Vertex* v,
					Delaunay::Mesh::Mesh& mesh)
{
  // split Triangle t into 3 new triangles using vertex v, remove t from the
  // triangle set and add the 3 new triangles to the triangle set
  const Mesh::Vertex& A = t->A();
  const Mesh::Vertex& B = t->B();
  const Mesh::Vertex& C = t->C();
  const Mesh::Vertex& D = *v;

  const Mesh::Edge& AB = t->AB();
  const Mesh::Edge& BC = t->BC();
  const Mesh::Edge& AC = t->AC();

  const Mesh::Edge& AD =*(this->GetEdges(mesh).emplace(A,D)).first;
  const Mesh::Edge& BD =*(this->GetEdges(mesh).emplace(B,D)).first;
  const Mesh::Edge& CD =*(this->GetEdges(mesh).emplace(C,D)).first;

  this->GetTriangles(mesh).emplace(AB,BD,AD);
  this->GetTriangles(mesh).emplace(AC,CD,AD);
  this->GetTriangles(mesh).emplace(BC,CD,BD);

  this->GetTriangles(mesh).erase(*t);

  std::set<const Mesh::Edge*> edges;
  edges.insert(&AB);
  edges.insert(&BC);
  edges.insert(&AC);

  LegalizeEdges(v, edges, mesh);
}

void DelaunayDiscretizer::ExtendMesh(const Vertex* v,
				     Delaunay::Mesh::Mesh& mesh)
{
  // append vertex v, which is exterior to the extant mesh, to the mesh

  double minPerimeterToArea = std::numeric_limits<double>::max();
  const Mesh::Edge* minEdge = nullptr;

  const Vertex* vlast = nullptr;
  const Vertex* vi = &(*(this->GetVertices(mesh).begin()));

  if (vi == v)
    vi = &(*(++this->GetVertices(mesh).begin()));

  unsigned vtx_counter = 0;

  const Vertex* vfirst = vi;

  do
  {
    Mesh::EdgeSet::iterator edge = vi->edges.begin();
    while ((*edge)->triangles.size() != 1 ||
	   vlast == &((*edge)->A()) || vlast == &((*edge)->B()))
    {
      edge++;
    }

    bool legal = false;
    {
      // check if a new triangle with this edge and vertex is legal
      const Vertex* va = &((*edge)->A());
      const Vertex* vb = &((*edge)->B());
      const Vertex* vopp;
      const Mesh::Triangle* t = *((*edge)->triangles.begin());
      vopp = &(t->A());
      if (vopp == &((*edge)->A()) || vopp == &((*edge)->B()))
  	vopp = &(t->B());
      if (vopp == &((*edge)->A()) || vopp == &((*edge)->B()))
  	vopp = &(t->C());

      bool b1 = (v->x-vb->x)*(va->y-vb->y) - (va->x-vb->x)*(v->y-vb->y) < 0.;
      bool b2 = (vopp->x-vb->x)*(va->y-vb->y) - (va->x-vb->x)*(vopp->y-vb->y) < 0.;
      legal = (b1 != b2);
    }

    if (legal)
    {
      const Vertex& A = (*edge)->A();
      const Vertex& B = (*edge)->B();
      const Vertex& C = *v;
      double perimeter = Distance(A,B) + Distance(A,C) + Distance(B,C);
      double area = fabs(A.x*(B.y-C.y) + B.x*(C.y-A.y) + C.x*(A.y-B.y))*.5;
      double perimeterToArea = perimeter/area;

      if (perimeterToArea < minPerimeterToArea)
  	minEdge = (*edge);
    }

    vlast = vi;
    vi = (vi == &((*edge)->A()) ? &((*edge)->B()) : &((*edge)->A()));
  }
  while (vi != vfirst);

  // Edges, triangles should be returned via a convenience method in Mesher
  const Mesh::Edge& va = *(this->GetEdges(mesh)
			   .emplace(*v, minEdge->A())).first;
  const Mesh::Edge& vb = *(this->GetEdges(mesh)
			   .emplace(*v, minEdge->B())).first;
  const Mesh::Triangle& t = *(this->GetTriangles(mesh).emplace(va,vb,*minEdge)).first;
  this->GetVertices(mesh).insert(*v);

  {
    const Vertex& A = t.A();
    const Vertex& B = t.B();
    const Vertex& C = t.C();

    std::set<const Mesh::Edge*> edges;
    for (Mesh::EdgeSet::iterator edge=A.edges.begin();edge!=A.edges.end();++edge)
      edges.insert(*edge);
    for (Mesh::EdgeSet::iterator edge=B.edges.begin();edge!=B.edges.end();++edge)
      edges.insert(*edge);
    for (Mesh::EdgeSet::iterator edge=C.edges.begin();edge!=C.edges.end();++edge)
      edges.insert(*edge);

    LegalizeEdges(v,edges,mesh);
  }

  return;
}

void DelaunayDiscretizer::LegalizeEdges(const Vertex* v,
					std::set<const Mesh::Edge*>& edges,
					Delaunay::Mesh::Mesh& mesh)
{
  if (edges.empty())
  {
    return;
  }

  const Mesh::Edge* edge = *(edges.begin());
  edges.erase(edges.begin());

  assert(edge->triangles.size()!=0);
  assert(edge->triangles.size()<=2);

  if (edge->triangles.size() == 1)
  {
    return LegalizeEdges(v,edges,mesh);
  }

  const Mesh::Triangle* t1 = *(edge->triangles.begin());
  const Mesh::Triangle* t2 = *(++(edge->triangles.begin()));

  const Vertex* I = &(edge->A());
  const Vertex* J = &(edge->B());
  const Vertex* K = &(t1->A());
  if (K == I || K == J)
    K = &(t1->B());
  if (K == I || K == J)
    K = &(t1->C());
  const Vertex* L = &(t2->A());
  if (L == I || L == J)
    L = &(t2->B());
  if (L == I || L == J)
    L = &(t2->C());

  const Mesh::Edge* ik = nullptr;
  const Mesh::Edge* jk = nullptr;
  const Mesh::Edge* il = nullptr;
  const Mesh::Edge* jl = nullptr;

  bool isLegal = true;

  const Point* C = &(t1->circumcenter);

  double d2 = (C->x-L->x)*(C->x-L->x) + (C->y-L->y)*(C->y-L->y);
  if (v == L)
  {
    if (d2 + EPSILON < t1->circumradius*t1->circumradius)
    {
      isLegal = false;
      for (Mesh::EdgeSet::iterator edge=I->edges.begin();edge!=I->edges.end();++edge)
	if (&((*edge)->A()) == K || &((*edge)->B()) == K)
	{
	  ik = *edge;
	  edges.insert(*edge);
	}
      for (Mesh::EdgeSet::iterator edge=J->edges.begin();edge!=J->edges.end();++edge)
	if (&((*edge)->A()) == K || &((*edge)->B()) == K)
	{
	  jk = *edge;
	  edges.insert(*edge);
	}
      il = &(*(this->GetEdges(mesh).emplace(*I, *L)).first);
      jl = &(*(this->GetEdges(mesh).emplace(*J, *L)).first);
    }
  }

  if (v == K)
  {
    C = &(t2->circumcenter);
    d2 = (C->x-K->x)*(C->x-K->x) + (C->y-K->y)*(C->y-K->y);
    if (d2 + EPSILON < t2->circumradius*t2->circumradius)
    {
      isLegal = false;

      for (Mesh::EdgeSet::iterator edge=I->edges.begin();edge!=I->edges.end();++edge)
	if (&((*edge)->A()) == L || &((*edge)->B()) == L)
	{
	  il = *edge;
	  edges.insert(*edge);
	}
      for (Mesh::EdgeSet::iterator edge=J->edges.begin();edge!=J->edges.end();++edge)
	if (&((*edge)->A()) == L || &((*edge)->B()) == L)
	{
	  jl = *edge;
	  edges.insert(*edge);
	}
      ik = &(*(this->GetEdges(mesh).emplace(*I, *K)).first);
      jk = &(*(this->GetEdges(mesh).emplace(*J, *K)).first);
    }
  }

  if (!isLegal)
  {
    const Mesh::Edge* kl = &(*(this->GetEdges(mesh).emplace(*K, *L)).first);

    this->GetEdges(mesh).emplace(*kl);
    this->GetTriangles(mesh).erase(*t1);
    this->GetTriangles(mesh).erase(*t2);
    this->GetEdges(mesh).erase(*edge);
    this->GetTriangles(mesh).emplace(*ik, *kl, *il);
    this->GetTriangles(mesh).emplace(*jk, *kl, *jl);
  }
  return LegalizeEdges(v,edges,mesh);
}

bool DelaunayDiscretizer::TestDelaunayCondition(Mesh::TriangleSet& illegalTriangles, const Delaunay::Mesh::Mesh& mesh) const
{
  for (auto edge=mesh.GetEdges().begin();edge!=mesh.GetEdges().end();++edge)
  {
    if ((*edge).triangles.size() < 2)
      continue;

    const Mesh::Triangle* t1 = *((*edge).triangles.begin());
    const Mesh::Triangle* t2 = *(++((*edge).triangles.begin()));

    const Vertex* I = &((*edge).A());
    const Vertex* J = &((*edge).B());
    const Vertex* K = &(t1->A());
    if (K == I || K == J)
      K = &(t1->B());
    if (K == I || K == J)
      K = &(t1->C());
    const Vertex* L = &(t2->A());
    if (L == I || L == J)
      L = &(t2->B());
    if (L == I || L == J)
      L = &(t2->C());

    bool isLegal = true;

    const Point* C = &(t1->circumcenter);

    double d2 = (C->x-L->x)*(C->x-L->x) + (C->y-L->y)*(C->y-L->y);
    if (d2 + EPSILON < t1->circumradius*t1->circumradius)
    {
      isLegal = false;
    }

    C = &(t2->circumcenter);
    d2 = (C->x-K->x)*(C->x-K->x) + (C->y-K->y)*(C->y-K->y);
    if (d2  + EPSILON < t2->circumradius*t2->circumradius)
    {
      isLegal = false;
    }

    if (!isLegal)
    {
      illegalTriangles.insert(t1);
      illegalTriangles.insert(t2);
    }
  }
  // if (!illegalTriangles.empty())
  //   throw(std::domain_error("Illegal triangles"));
  return illegalTriangles.empty();
}

}
}

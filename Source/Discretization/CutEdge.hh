/******************************************************************************

  This source file is part of the Delaunay project.

  Copyright T.J. Corona

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#ifndef DELAUNAY_DISCRETIZATION_CUTEDGE_HH
#define DELAUNAY_DISCRETIZATION_CUTEDGE_HH

#include "Discretization/Export.hh"

#include "Shape/Polygon.hh"
#include "Mesh/Mesher.hh"

namespace Delaunay
{
namespace Discretization
{

class DELAUNAYDISCRETIZATION_EXPORT CutEdge : public Mesh::Mesher
{
public:
  CutEdge() {}

  std::set<const Mesh::Edge*> operator()(const Shape::LineSegment&,Mesh::Mesh&);

private:

  const Mesh::Edge* CutEdgeInTriangle(const Shape::LineSegment&,
				      const Mesh::Triangle&, Mesh::Mesh&);

  std::set<const Mesh::Triangle*> FindContainingTriangles(const Shape::LineSegment&, Delaunay::Mesh::Mesh&) const;

  std::set<const Mesh::Edge*> edgesToRemove;
  std::set<const Mesh::Triangle*> trianglesToRemove;
};

}
}

#endif

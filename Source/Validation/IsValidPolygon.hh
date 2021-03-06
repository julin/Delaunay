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

#ifndef DELAUNAY_VALIDATION_ISVALIDPOLYGON_HH
#define DELAUNAY_VALIDATION_ISVALIDPOLYGON_HH

#include "Validation/Export.hh"

#include "Shape/Polygon.hh"

namespace Delaunay
{
namespace Validation
{

class DELAUNAYVALIDATION_EXPORT IsValidPolygon
{
public:
  IsValidPolygon() {}

  bool operator()(const Delaunay::Shape::Polygon&) const;
};

}
}

#endif

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

#include "QuadraticFunction.hh"

namespace Delaunay
{
namespace Misc
{

QuadraticFunction::QuadraticFunction()
{
  coefficients[0] = 1.;
  coefficients[1] = 0.;
  coefficients[2] = 0.;
}

double QuadraticFunction::operator() (double x) const
{
  return coefficients[0]*x*x + coefficients[1]*x + coefficients[2];
}

void QuadraticFunction::SetCoefficients(double a, double b, double c)
{
  coefficients[0] = a;
  coefficients[1] = b;
  coefficients[2] = c;
}

void QuadraticFunction::SetCoefficients(double* coeff)
{
  SetCoefficients(coeff[0],coeff[1],coeff[2]);
}

}
}

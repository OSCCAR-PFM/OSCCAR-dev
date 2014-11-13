#include "vector2.H"
#include "IOstreams.H"

using namespace Foam;

int main()
{
    Info<< vector2::zero << endl
        << vector2::one << endl
        << vector2::dim << endl
        << vector2::rank << endl;

    
    //vector d(0.5, 0.5);
    vector2 d = 0.5*vector2::one;
    d /= mag(d);

    vector2 dSmall = (1e-100)*d;
    dSmall /= mag(dSmall);

    Info<< (dSmall - d) << endl;

    d *= 4.0;

    Info<< d << endl;

    Info<< d + d << endl;

    Info<< magSqr(d) << endl;

    //vector d2(0.5, -0.5);
    vector2 d2 = vector2::one;
    Info<< cmptMax(d2) << " "
        << cmptSum(d2) << " "
        << cmptProduct(d2) << " "
        << cmptMag(d2)
        << endl;
    Info<< min(d, d2) << endl;
    return 0;
}

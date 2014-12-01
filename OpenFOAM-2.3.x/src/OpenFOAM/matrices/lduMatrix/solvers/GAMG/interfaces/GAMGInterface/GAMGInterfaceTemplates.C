/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "GAMGInterface.H"
#include "scalarCoeffField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::Field<Type> > Foam::GAMGInterface::interfaceInternalField
(
    const UList<Type>& iF
) const
{
    tmp<Field<Type> > tresult(new Field<Type>(size()));
    interfaceInternalField(iF, tresult());
    return tresult;
}


template<class Type>
void Foam::GAMGInterface::interfaceInternalField
(
    const UList<Type>& iF,
    List<Type>& result
) const
{
    result.setSize(size());

    forAll(result, elemI)
    {
        result[elemI] = iF[faceCells_[elemI]];
    }
}


template<class Type>
Foam::tmp<Foam::CoeffField<Type> > Foam::GAMGInterface::agglomerateBlockCoeffs
(
    const Foam::CoeffField<Type>& fineCoeffs
) const
{
    tmp<CoeffField<Type> > tcoarseCoeffs(new CoeffField<Type>(size()));
    CoeffField<Type>& coarseCoeffs = tcoarseCoeffs();

    typedef CoeffField<Type> TypeCoeffField;

    typedef typename TypeCoeffField::linearTypeField linearTypeField;
    typedef typename TypeCoeffField::squareTypeField squareTypeField;

    if (fineCoeffs.activeType() == blockCoeffBase::SQUARE)
    {
        squareTypeField& activeCoarseCoeffs = coarseCoeffs.asSquare();
        const squareTypeField& activeFineCoeffs = fineCoeffs.asSquare();

        activeCoarseCoeffs *= 0.0;

        // Added weights to account for non-integral matching
        forAll(faceRestrictAddressing_, ffi)
        {
            //activeCoarseCoeffs[faceRestrictAddressing_[ffi]] +=
            //    restrictWeights_[ffi]*activeFineCoeffs[fineAddressing_[ffi]];
            // FIXME?
            activeCoarseCoeffs[faceRestrictAddressing_[ffi]] +=
            	activeFineCoeffs[ffi];
        }
    }
    else if (fineCoeffs.activeType() == blockCoeffBase::LINEAR)
    {
        linearTypeField& activeCoarseCoeffs = coarseCoeffs.asLinear();
        const linearTypeField& activeFineCoeffs = fineCoeffs.asLinear();

        activeCoarseCoeffs *= 0.0;

        // Added weights to account for non-integral matching
        forAll(faceRestrictAddressing_, ffi)
        {
            //activeCoarseCoeffs[faceRestrictAddressing_[ffi]] +=
            //    restrictWeights_[ffi]*activeFineCoeffs[fineAddressing_[ffi]];
            // FIXME?
            activeCoarseCoeffs[faceRestrictAddressing_[ffi]] +=
            	activeFineCoeffs[ffi];
        }
    }

    return tcoarseCoeffs;
}

// ************************************************************************* //

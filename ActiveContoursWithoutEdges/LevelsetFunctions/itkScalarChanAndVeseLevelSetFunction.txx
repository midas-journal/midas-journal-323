#ifndef __itkScalarChanAndVeseLevelSetFunction_txx_
#define __itkScalarChanAndVeseLevelSetFunction_txx_

#include "itkScalarChanAndVeseLevelSetFunction.h"

namespace itk {

/* Calculates the numerator and denominator for c_i for each region. As part of
the optimization, it is called once at the beginning of the code, and then the
cNum and cDen are updated during the evolution without iterating through the
entire image. */
template < class TInputImage, class TFeatureImage, class TSharedData >
void
ScalarChanAndVeseLevelSetFunction< TInputImage, TFeatureImage, TSharedData >
::ComputeParameters()
{
  unsigned int fId = this->m_FunctionId;

  for( unsigned int i = 0; i < this->sharedData->FunctionCount; i++ )
  {
    this->sharedData->cDens[i] = 0;
    this->sharedData->cNums[i] = 0;
    this->sharedData->cVals[i] = 0;
    this->sharedData->cBDen[i] = 0;
    this->sharedData->cBNum[i] = 0;
    this->sharedData->cB[i] = 0;
  }
	
	FeatureImageConstPointer featureImage = this->m_FeatureImage;

  ImageIteratorType It( this->sharedData->hVals[fId],
    this->sharedData->hVals[fId]->GetLargestPossibleRegion() );
	
  FeaturePixelType itFeatureVal;
  FeatureIndexType itFeatureIndex;
  InputIndexType itInputIndex, inputIndex;
  InputPixelType hVal, inputVal;
  ListPixelType L;

  for( It.GoToBegin(); !It.IsAtEnd(); ++It )
  {
    inputVal = It.Get();
    inputIndex = It.GetIndex();

		itFeatureIndex = this->sharedData->GetFeatureIndex( fId, inputIndex );
		itFeatureVal = featureImage->GetPixel( itFeatureIndex );

    L = this->sharedData->lImage->GetPixel( itFeatureIndex );

    bool inBgrnd = true; // assume the pixel is in background
    for( ListPixelConstIterator it = L.begin(); it != L.end(); ++it )
    {
      itInputIndex = this->sharedData->GetIndex( *it, itFeatureIndex );
      hVal = this->sharedData->hVals[*it]->GetPixel( itInputIndex );

      if ( hVal > 0.5 )
      {
        // inside the level-set function
        this->sharedData->cNums[*it] += itFeatureVal;
        this->sharedData->cDens[*it] += 1.;
        inBgrnd = false;
      }
    }
		
    // if the pixel belongs to the background
    if ( inBgrnd )
    {
      this->sharedData->cBNum[fId] += itFeatureVal;
      this->sharedData->cBDen[fId] += 1.;
    }
  }
}

template < class TInputImage, class TFeatureImage, class TSharedData >
double
ScalarChanAndVeseLevelSetFunction< TInputImage, TFeatureImage, TSharedData >::
computeInternalTerm( const FeaturePixelType& iValue,
  const FeatureIndexType& iIdx, const unsigned int& fId )
{
  double cVals = this->sharedData->cVals[fId];
  double t = ( iValue - cVals );
  return this->m_Lambda1 * t * t;
}

template < class TInputImage, class TFeatureImage, class TSharedData >
double
ScalarChanAndVeseLevelSetFunction< TInputImage, TFeatureImage, TSharedData >
::computeExternalTerm( const FeaturePixelType& iValue,
    const FeatureIndexType& iIdx, const unsigned int& pr )
{
  unsigned int fId = this->m_FunctionId;
  double cBgrnd = this->sharedData->cB[fId]; // bgrnd

  return pr * ( iValue - cBgrnd ) * ( iValue - cBgrnd );
}

/* Performs the narrow-band update of the Heaviside function for each voxel. The
characteristic function of each region is recomputed (note the shared
data which contains information from the other level sets). Using the
new H values, the previous c_i are updated. */
template < class TInputImage, class TFeatureImage, class TSharedData >
void
ScalarChanAndVeseLevelSetFunction< TInputImage, TFeatureImage, TSharedData >
::UpdatePixel( const unsigned int& idx, NeighborhoodIterator< TInputImage >
&iterator, InputPixelType &newValue, bool &status )
{
  unsigned int i = this->m_FunctionId;

  // For each affected h val: h val = new hval (this will dirty some cvals)
  InputIndexType inputIndex = iterator.GetIndex(idx);
  FeatureIndexType featureIndex =
		this->sharedData->GetFeatureIndex( i, inputIndex );

  FeaturePixelType featureVal =
    this->GetFeatureImage()->GetPixel( featureIndex );

  double oldH = this->sharedData->hVals[i]->GetPixel( inputIndex );
  double newH = this->calculateH( newValue );

  // Check if it is in other foreground
  ListPixelType L = this->sharedData->lImage->GetPixel( featureIndex );
  InputIndexType itInputIndex;
  float hVal;

  bool inBgrnd = true; // assume the pixel is in background
  for( ListPixelType::const_iterator it = L.begin(); it != L.end(); ++it )
  {
    itInputIndex = this->sharedData->GetIndex( *it, featureIndex );
    hVal = this->sharedData->hVals[*it]->GetPixel( itInputIndex );

    if ( ( hVal > 0.5 ) && ( *it != i ) )
    {
      inBgrnd = false; // belongs to foreground elsewhere
    }
  }

  // if pixel belonged to current foreground but not anymore so
  if ( ( oldH > 0.5 ) && ( newH <= 0.5 ) )
  {
    this->sharedData->cDens[i]--;
    this->sharedData->cNums[i] -= featureVal;

    if ( inBgrnd )
    {
      this->sharedData->cBNum[i] += featureVal;
      this->sharedData->cBDen[i]++;
    }
  }

  // if pixel entered the foreground
  if ( ( oldH <= 0.5 ) & ( newH > 0.5 ) )
  {
    this->sharedData->cDens[i]++;
    this->sharedData->cNums[i] += featureVal;

    if ( inBgrnd )
    {
      this->sharedData->cBNum[i] -= featureVal;
      this->sharedData->cBDen[i]--;
    }
  }

  this->sharedData->hVals[i]->SetPixel( inputIndex, newH );
}


} // end namespace itk
#endif

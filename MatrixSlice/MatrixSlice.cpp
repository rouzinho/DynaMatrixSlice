

// CEDAR INCLUDES
#include "MatrixSlice.h"
#include "cedar/processing/typecheck/Matrix.h"
#include "cedar/processing/ElementDeclaration.h"
#include "cedar/processing/ExternalData.h"
#include "cedar/processing/DataSlot.h"
#include "cedar/auxiliaries/math/tools.h"
#include "cedar/auxiliaries/MatData.h"
#include "cedar/auxiliaries/assert.h"

// SYSTEM INCLUDES
#include <algorithm>


//----------------------------------------------------------------------------------------------------------------------
// register the class
//----------------------------------------------------------------------------------------------------------------------
/*
namespace
{
  bool declare()
  {
    using cedar::proc::ElementDeclarationPtr;
    using cedar::proc::ElementDeclarationTemplate;

    ElementDeclarationPtr declaration
    (
      new ElementDeclarationTemplate<cedar::proc::steps::MatrixSlice>
      (
        "Arrays",
        "cedar.processing.MatrixSlice"
      )
    );
    declaration->setIconPath(":/steps/matrix_slice.svg");
    declaration->setDescription
    (
      "Extracts a subregion of a matrix."
    );

    declaration->declare();

    return true;
  }

  bool declared = declare();
}
*/
//----------------------------------------------------------------------------------------------------------------------
// anchor type enum class
//----------------------------------------------------------------------------------------------------------------------

cedar::aux::EnumType<MatrixSlice::AnchorType>
  MatrixSlice::AnchorType::mType("cedar::proc::steps::MatrixSlice::AnchorType::");

#ifndef CEDAR_COMPILER_MSVC
const MatrixSlice::AnchorType::Id MatrixSlice::AnchorType::Absolute;
const MatrixSlice::AnchorType::Id MatrixSlice::AnchorType::Center;
#endif // CEDAR_COMPILER_MSVC

//----------------------------------------------------------------------------------------------------------------------
// constructors and destructor
//----------------------------------------------------------------------------------------------------------------------

MatrixSlice::MatrixSlice()
:
mOutput(new cedar::aux::MatData(cv::Mat())),
_mAnchorType
(
  new cedar::aux::EnumParameter
  (
    this,
    "anchor type",
    MatrixSlice::AnchorType::typePtr(),
    MatrixSlice::AnchorType::Absolute
  )
),
_mRangeLower
(
  new cedar::aux::UIntVectorParameter
  (
    this,
    "range lower",
    2,
    10,
    cedar::aux::UIntVectorParameter::LimitType::positiveZero()
  )
),
_mRangeUpper
(
  new cedar::aux::UIntVectorParameter
  (
    this,
    "range upper",
    2,
    15,
    cedar::aux::UIntVectorParameter::LimitType::positiveZero()
  )
)
{
  auto input = this->declareInput("matrix");
  auto srentry = this->declareInput("entry",false);

  cedar::proc::typecheck::Matrix input_check;
  cedar::proc::typecheck::Matrix input_entry;
  input_check.addAcceptedDimensionalityRange(1, 16);
  input_entry.addAcceptedDimensionalityRange(0, 1);
  input->setCheck(input_check);
  srentry->setCheck(input_entry);
  l = 0;
  u = 0;
  first = true;

  this->declareOutput("slice", mOutput);

  QObject::connect(this->_mAnchorType.get(), SIGNAL(valueChanged()), this, SLOT(rangeChanged()), Qt::DirectConnection);
  QObject::connect(this->_mRangeLower.get(), SIGNAL(valueChanged()), this, SLOT(rangeChanged()), Qt::DirectConnection);
  QObject::connect(this->_mRangeUpper.get(), SIGNAL(valueChanged()), this, SLOT(rangeChanged()), Qt::DirectConnection);
}

//----------------------------------------------------------------------------------------------------------------------
// methods
//----------------------------------------------------------------------------------------------------------------------

void MatrixSlice::setAnchor(cedar::aux::EnumId anchor)
{
  this->_mAnchorType->setValue(anchor);
}

cv::Range MatrixSlice::getRange(unsigned int dimension) const
{
  cv::Range range;
  CEDAR_ASSERT(dimension < this->_mRangeLower->size());
  CEDAR_ASSERT(dimension < this->_mRangeUpper->size());

  range.start = static_cast<int>(this->_mRangeLower->at(dimension));
  range.end = static_cast<int>(this->_mRangeUpper->at(dimension));

  return range;
}

void MatrixSlice::setRanges(const std::vector<cv::Range>& ranges)
{
  cedar::proc::Step::WriteLocker locker(this);

  CEDAR_ASSERT(ranges.size() == this->_mRangeLower->size());
  CEDAR_ASSERT(ranges.size() == this->_mRangeUpper->size());

  bool lower_blocked = this->_mRangeLower->blockSignals(true);
  bool upper_blocked = this->_mRangeUpper->blockSignals(true);

  for (size_t d = 0; d < ranges.size(); ++d)
  {
    this->_mRangeUpper->setValue(d, ranges.at(d).end);
    this->_mRangeLower->setValue(d, ranges.at(d).start);
  }

  this->_mRangeLower->blockSignals(lower_blocked);
  this->_mRangeUpper->blockSignals(upper_blocked);

  locker.unlock();

  this->rangeChanged();
}

void MatrixSlice::readConfiguration(const cedar::aux::ConfigurationNode& node)
{
  cedar::proc::Step::readConfiguration(node);

  this->mStoredLimits.clear();
  for (size_t i = 0; i < std::min(this->_mRangeLower->size(), this->_mRangeUpper->size()); ++i)
  {
    this->mStoredLimits.push_back
    (
      cedar::aux::math::Limits<unsigned int>(this->_mRangeLower->at(i), this->_mRangeUpper->at(i))
    );
  }
}

void MatrixSlice::inputConnectionChanged(const std::string& inputName)
{

  if(inputName == "matrix")
  {
    CEDAR_DEBUG_ASSERT(inputName == "matrix");
    this->mInput = boost::dynamic_pointer_cast<cedar::aux::ConstMatData>(this->getInput(inputName));


      if (this->mInput)
      {
        this->mOutput->copyAnnotationsFrom(this->mInput);
        this->updateDimensionality();

      }
  }

  if(inputName == "entry")
  {
   // inputConnectionChanged is called for any input. The incoming data is assigned depending on the name of the input
   this->mInputEntry = boost::dynamic_pointer_cast<cedar::aux::ConstMatData>(this->getInput(inputName));
  }

}

void MatrixSlice::nothingChanged()
{

}


void MatrixSlice::updateDimensionality()
{
  if (!this->mInput)
  {
    return;
  }

  unsigned int matrix_dim = cedar::aux::math::getDimensionalityOf(this->mInput->getData());
  unsigned int old_dim = this->_mRangeLower->size();
  this->_mRangeLower->resize(matrix_dim);
  this->_mRangeUpper->resize(matrix_dim);

  for (unsigned int d = old_dim; d < matrix_dim && d < this->mStoredLimits.size(); ++d)
  {
    const cedar::aux::math::Limits<unsigned int>& limits = this->mStoredLimits.at(d);

    bool lower_blocked = this->_mRangeLower->blockSignals(true);
    bool upper_blocked = this->_mRangeUpper->blockSignals(true);

    CEDAR_DEBUG_ASSERT(d < this->_mRangeLower->size());
    CEDAR_DEBUG_ASSERT(d < this->_mRangeUpper->size());

    this->_mRangeLower->setValue(d, limits.getLower());
    this->_mRangeUpper->setValue(d, limits.getUpper());

    this->_mRangeLower->blockSignals(lower_blocked);
    this->_mRangeUpper->blockSignals(upper_blocked);
  }

  if (this->allInputsValid())
  {
    this->allocateOutputMatrix();
  }
}

void MatrixSlice::allocateOutputMatrix()
{
  //cedar::proc::Step::ReadLocker locker(this);
  if (!this->mInput || this->mInput->isEmpty())
  {
    return;
  }

  //std::cout << "ALLOCATE MATRIX" << '\n';

  const cv::Mat& input = this->mInput->getData();
  unsigned int dimensionality = cedar::aux::math::getDimensionalityOf(input);

  CEDAR_DEBUG_ASSERT(dimensionality == this->_mRangeLower->size());
  CEDAR_DEBUG_ASSERT(dimensionality == this->_mRangeUpper->size());
  CEDAR_DEBUG_ASSERT(dimensionality > 0);

  mRanges.clear();
  mRanges.resize(dimensionality);
  std::vector<int> sizes;
  sizes.resize(dimensionality, 1);

  auto apply_range = [&] (unsigned int input_dimension, unsigned int output_dimension, const cv::Mat& input)
  {
    CEDAR_DEBUG_ASSERT(output_dimension < this->_mRangeLower->size());
    CEDAR_DEBUG_ASSERT(output_dimension < this->_mRangeUpper->size());

    int lower;
    int upper;

    if( output_dimension == 0)
    {
      lower = static_cast<int>(this->_mRangeLower->at(output_dimension));
      upper = static_cast<int>(this->_mRangeUpper->at(output_dimension));
    }
    else
    {
      lower = l;
      upper = u;
    }

    int dim_size = input.size[input_dimension];


    switch (this->_mAnchorType->getValue())
    {
      case AnchorType::Absolute:
        // nothing to do
        break;

      case AnchorType::Center:
      {
        // here we see "upper" as the size of the region to cut out
        // "lower" is seen as an offset from the top-left of a center-aligned rectangle of size "upper"
        int width = upper / 2;
        lower = dim_size/2 - (upper - width) + lower; // upper - width to avoid issues with integer division
        upper = dim_size/2 + width;
        break;
      }
    }

    // ensure that lower < upper, and that the interval isn't size 0 (i.e., lower != upper)
    if (lower > upper)
    {
      std::swap(lower, upper);
    }
    else if (lower == upper)
    {
      if (upper < dim_size)
      {
        upper += 1;
      }
      else
      {
        // this assertion should only fail if the matrix size is 0 in dimension d, which should not be possible
        CEDAR_DEBUG_NON_CRITICAL_ASSERT(lower > 0);
        lower -= 1;
      }
    }

    // make sure that lower and upper don't exceed the matrix size
    lower = cedar::aux::math::Limits<int>::limit(lower, 0, dim_size - 1);
    upper = cedar::aux::math::Limits<int>::limit(upper, 0, dim_size);

    this->mRanges.at(input_dimension) = cv::Range(lower, upper);
    sizes.at(input_dimension) = upper - lower;
  };

  if (dimensionality > 1)
  {
    for (unsigned int d = 0; d < dimensionality; ++d)
    {
      apply_range(d, d, input);
    }
  }
  else // case: dimensionality <= 1
  {
    int slice_dim = 0;
    int other_dim = 1;
    mRanges.resize(2);
    sizes.resize(2);

    if (input.rows == 1)
    {
      slice_dim = 1;
      other_dim = 0;
    }

    apply_range(slice_dim, 0, input);

    mRanges[other_dim] = cv::Range::all();
    sizes[other_dim] = 1;
  }

  // preallocate the appropriate output matrix
  CEDAR_DEBUG_ASSERT(sizes.size() > 0);
  cv::Mat output = cv::Mat(static_cast<int>(sizes.size()), &sizes.front(), input.type(), cv::Scalar(0));
  cv::Mat old_output = this->mOutput->getData();
  this->mOutput->setData(output);

  //locker.unlock();

  if (output.type() != old_output.type() || !cedar::aux::math::matrixSizesEqual(output, old_output))
  {
    this->emitOutputPropertiesChangedSignal("slice");
  }
}

void MatrixSlice::rangeChanged()
{
  if (!this->mInput || !this->allInputsValid())
  {
    return;
  }
  this->allocateOutputMatrix();

  this->resetState();

  this->onTrigger();
}

void MatrixSlice::compute(const cedar::proc::Arguments&)
{
  const cv::Mat& input = this->mInput->getData();
  if(this->mInputEntry)
  {
    const cv::Mat& inputEntry = this->mInputEntry->getData();
    float d;
    int i;

    cv::Size s = inputEntry.size();
    for(i = 0;i < s.height;i++)
    {
      d = inputEntry.at<float>(i);
      if(d > 0.01)
      {
        if(first == true)
        {
          l = i;
          first = false;
        }
        u = i;
      }
    }
    first = true;
    this->rangeChanged();
  }

  cv::Mat& output = this->mOutput->getData();



  //CEDAR_DEBUG_ASSERT(this->_mRangeLower->size() == cedar::aux::math::getDimensionalityOf(input));
  //CEDAR_DEBUG_ASSERT(this->_mRangeUpper->size() == cedar::aux::math::getDimensionalityOf(input));

  output = input(&mRanges.front()).clone();
}

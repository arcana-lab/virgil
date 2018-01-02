/*
 * Copyright 2017 Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The TaskFuture class.
 */
#pragma once

namespace MARC{

  template <class ValueType>
  class SequenceOfValues {
    public:

      /*
       * Constructor.
       */
      SequenceOfValues (uint32_t numberOfElements);

      /*
       * Copy operator.
       */
      SequenceOfValues& operator= (const SequenceOfValues& other);

      /*
       * Copy constructor.
       */
      SequenceOfValues (const SequenceOfValues& other);

      /*
       * Deconstructor.
       */
      ~SequenceOfValues (void);

      /*
       * Not assignable.
       */
      SequenceOfValues (SequenceOfValues&& other) = delete;
      SequenceOfValues& operator= (SequenceOfValues&& other) = delete;

      /*
       * Object fields.
       */
      mutable std::mutex mutex;
      ValueType *values;
      uint32_t numberOfValues;
  };

}

template <class ValueType>
MARC::SequenceOfValues<ValueType>::SequenceOfValues (uint32_t numberOfElements){
  this->numberOfValues = numberOfElements;
  this->values = new ValueType[this->numberOfValues];

  return ;
}

template <class ValueType>
MARC::SequenceOfValues<ValueType>& MARC::SequenceOfValues<ValueType>::operator= (const MARC::SequenceOfValues<ValueType>& other){
  if (this != &other){
    if (this->numberOfValues != other.numberOfValues){
      abort();
    }
    std::copy(other.values, other.values + other.numberOfValues, this->values);
  }

  return *this;
}

template <class ValueType>
MARC::SequenceOfValues<ValueType>::SequenceOfValues (const MARC::SequenceOfValues<ValueType> & other)
  : MARC::SequenceOfValues<ValueType>::SequenceOfValues(other.numberOfValues)
  {
  std::copy(other.values, other.values + other.numberOfValues, this->values);

  return ;
}

template <class ValueType>
MARC::SequenceOfValues<ValueType>::~SequenceOfValues (void){
  delete[] this->values;

  return ;
}

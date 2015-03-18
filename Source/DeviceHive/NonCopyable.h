/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   19.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "Namespace.h"

namespace noncopyable_  // protection from unintended ADL
{
  class noncopyable
  {
   protected:
      noncopyable() {}
      ~noncopyable() {}
      
   private:  // emphasize the following members are private
      noncopyable( const noncopyable& );
      const noncopyable& operator=( const noncopyable& );
  };
}

typedef noncopyable_::noncopyable NonCopyable;

#include "NamespaceEnd.h"
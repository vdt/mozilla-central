/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsConverterInputStream.h"
#include "nsIInputStream.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"

#define CONVERTER_BUFFER_SIZE 8192

NS_IMPL_ISUPPORTS3(nsConverterInputStream, nsIConverterInputStream,
                   nsIUnicharInputStream, nsIUnicharLineInputStream)
    
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMETHODIMP
nsConverterInputStream::Init(nsIInputStream* aStream,
                             const char *aCharset,
                             PRInt32 aBufferSize,
                             PRUnichar aReplacementChar)
{
    if (!aCharset)
        aCharset = "UTF-8";

    nsresult rv;

    if (aBufferSize <=0) aBufferSize=CONVERTER_BUFFER_SIZE;
    
    // get the decoder
    nsCOMPtr<nsICharsetConverterManager> ccm =
        do_GetService(kCharsetConverterManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = ccm->GetUnicodeDecoder(aCharset ? aCharset : "ISO-8859-1", getter_AddRefs(mConverter));
    if (NS_FAILED(rv)) return rv;
 
    // set up our buffers
    rv = NS_NewByteBuffer(getter_AddRefs(mByteData), nsnull, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewUnicharBuffer(getter_AddRefs(mUnicharData), nsnull, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    mInput = aStream;
    mReplacementChar = aReplacementChar;
    
    return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::Close()
{
    nsresult rv = mInput ? mInput->Close() : NS_OK;
    PR_FREEIF(mLineBuffer);
    mInput = nsnull;
    mConverter = nsnull;
    mByteData = nsnull;
    mUnicharData = nsnull;
    return rv;
}

NS_IMETHODIMP
nsConverterInputStream::Read(PRUnichar* aBuf,
                             PRUint32 aCount,
                             PRUint32 *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 readCount = mUnicharDataLength - mUnicharDataOffset;
  if (0 == readCount) {
    // Fill the unichar buffer
    readCount = Fill(&mLastErrorCode);
    if (readCount == 0) {
      *aReadCount = 0;
      return mLastErrorCode;
    }
  }
  if (readCount > aCount) {
    readCount = aCount;
  }
  memcpy(aBuf, mUnicharData->GetBuffer() + mUnicharDataOffset,
         readCount * sizeof(PRUnichar));
  mUnicharDataOffset += readCount;
  *aReadCount = readCount;
  return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::ReadSegments(nsWriteUnicharSegmentFun aWriter,
                                     void* aClosure,
                                     PRUint32 aCount, PRUint32 *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 bytesToWrite = mUnicharDataLength - mUnicharDataOffset;
  nsresult rv;
  if (0 == bytesToWrite) {
    // Fill the unichar buffer
    bytesToWrite = Fill(&rv);
    if (bytesToWrite <= 0) {
      *aReadCount = 0;
      return rv;
    }
  }
  
  if (bytesToWrite > aCount)
    bytesToWrite = aCount;
  
  PRUint32 bytesWritten;
  PRUint32 totalBytesWritten = 0;

  while (bytesToWrite) {
    rv = aWriter(this, aClosure,
                 mUnicharData->GetBuffer() + mUnicharDataOffset,
                 totalBytesWritten, bytesToWrite, &bytesWritten);
    if (NS_FAILED(rv)) {
      // don't propagate errors to the caller
      break;
    }
    
    bytesToWrite -= bytesWritten;
    totalBytesWritten += bytesWritten;
    mUnicharDataOffset += bytesWritten;
    
  }

  *aReadCount = totalBytesWritten;

  return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::ReadString(PRUint32 aCount, nsAString& aString,
                                   PRUint32* aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 readCount = mUnicharDataLength - mUnicharDataOffset;
  if (0 == readCount) {
    // Fill the unichar buffer
    readCount = Fill(&mLastErrorCode);
    if (readCount == 0) {
      *aReadCount = 0;
      return mLastErrorCode;
    }
  }
  if (readCount > aCount) {
    readCount = aCount;
  }
  const PRUnichar* buf = reinterpret_cast<const PRUnichar*>(mUnicharData->GetBuffer() +
                                             mUnicharDataOffset);
  aString.Assign(buf, readCount);
  mUnicharDataOffset += readCount;
  *aReadCount = readCount;
  return NS_OK;
}

PRUint32
nsConverterInputStream::Fill(nsresult * aErrorCode)
{
  if (nsnull == mInput) {
    // We already closed the stream!
    *aErrorCode = NS_BASE_STREAM_CLOSED;
    return 0;
  }

  if (NS_FAILED(mLastErrorCode)) {
    // We failed to completely convert last time, and error-recovery
    // is disabled.  We will fare no better this time, so...
    *aErrorCode = mLastErrorCode;
    return 0;
  }
  
  // We assume a many to one conversion and are using equal sizes for
  // the two buffers.  However if an error happens at the very start
  // of a byte buffer we may end up in a situation where n bytes lead
  // to n+1 unicode chars.  Thus we need to keep track of the leftover
  // bytes as we convert.
  
  PRInt32 nb = mByteData->Fill(aErrorCode, mInput, mLeftOverBytes);
#if defined(DEBUG_bzbarsky) && 0
  for (unsigned int foo = 0; foo < mByteData->GetLength(); ++foo) {
    fprintf(stderr, "%c", mByteData->GetBuffer()[foo]);
  }
  fprintf(stderr, "\n");
#endif
  if (nb <= 0 && mLeftOverBytes == 0) {
    // No more data 
    *aErrorCode = NS_OK;
    return 0;
  }

  NS_ASSERTION(PRUint32(nb) + mLeftOverBytes == mByteData->GetLength(),
               "mByteData is lying to us somewhere");
  
  // Now convert as much of the byte buffer to unicode as possible
  mUnicharDataOffset = 0;
  mUnicharDataLength = 0;
  PRUint32 srcConsumed = 0;
  do {
    PRInt32 srcLen = mByteData->GetLength() - srcConsumed;
    PRInt32 dstLen = mUnicharData->GetBufferSize() - mUnicharDataLength;
    *aErrorCode = mConverter->Convert(mByteData->GetBuffer()+srcConsumed,
                                      &srcLen,
                                      mUnicharData->GetBuffer()+mUnicharDataLength,
                                      &dstLen);
    mUnicharDataLength += dstLen;
    // XXX if srcLen is negative, we want to drop the _first_ byte in
    // the erroneous byte sequence and try again.  This is not quite
    // possible right now -- see bug 160784
    srcConsumed += srcLen;
    if (NS_FAILED(*aErrorCode) && mReplacementChar) {
      NS_ASSERTION(0 < mUnicharData->GetBufferSize() - mUnicharDataLength,
                   "Decoder returned an error but filled the output buffer! "
                   "Should not happen.");
      mUnicharData->GetBuffer()[mUnicharDataLength++] = mReplacementChar;
      ++srcConsumed;
      // XXX this is needed to make sure we don't underrun our buffer;
      // bug 160784 again
      srcConsumed = NS_MAX<PRUint32>(srcConsumed, 0);
      mConverter->Reset();
    }
    NS_ASSERTION(srcConsumed <= mByteData->GetLength(),
                 "Whoa.  The converter should have returned NS_OK_UDEC_MOREINPUT before this point!");
  } while (mReplacementChar &&
           NS_FAILED(*aErrorCode) &&
           PRUint32(mUnicharData->GetBufferSize()) > mUnicharDataLength);

  mLeftOverBytes = mByteData->GetLength() - srcConsumed;

  return mUnicharDataLength;
}

NS_IMETHODIMP
nsConverterInputStream::ReadLine(nsAString& aLine, bool* aResult)
{
  if (!mLineBuffer) {
    nsresult rv = NS_InitLineBuffer(&mLineBuffer);
    if (NS_FAILED(rv)) return rv;
  }
  return NS_ReadLine(this, mLineBuffer, aLine, aResult);
}

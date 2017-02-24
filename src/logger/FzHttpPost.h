#ifdef WEBLOG_ENABLED

//Created by KVClassFactory on Wed May 20 11:31:27 2015
//Author: John Frankland,,,

#ifndef __FZHTTPPOST_H
#define __FZHTTPPOST_H

#include <string>
#include <curl/curl.h>

class FZHttpPost
{
   CURL*                          fCURL;                      //! interface to libcurl
   struct curl_httppost*          formpost;                   //!
   struct curl_httppost*          lastptr;                    //!

   protected:
   std::string			  fServerURL;                 //! URL of server including PHP script name
   std::string			  fDBUserName;                //! user name (="SlowControls" by default)
   std::string			  fDBCategory;                //! category (="SlowControls" by default)
   static int			  fNHdrElements;              //  number of extra key-value pairs added as 'header' infos
   static int			  fMaxPHP;                    //  PHP limit on number of key-value pairs in POST
   std::string			  fPHPArrayName;              //! name to be used for PHP array in POST
   std::string			  fPHPArrayFormatString;      //! string used to format name of each member of POST array
   int				  fPairsPerLine;              //! number of key-value pairs for each line of post
   int				  fLineNumber;                //! number of line in current post
   int 				  fPostPairs;                 //! total number of key-value pairs in current post

   bool CheckPHPLimit()
   {
      // Return kTRUE if adding another line of fPairsPerLine key-value pairs
      // would exceed the current limit of fMaxPHP pairs per POST
      return ((fPostPairs+fNHdrElements)>(fMaxPHP-fPairsPerLine));
   }
   void AddKeyValuePair(const std::string&,const std::string&);

   public:
   FZHttpPost();
   virtual ~FZHttpPost();

   void SetDBUserName(const std::string&);
   void SetDBCategory(const std::string&);
   static void SetPHPLimit(int);
   void SetPHPArrayName(const std::string&);
   void SetServerURL(const std::string&);
   const char* GetServerURL() const;

   void NewPost();
   void NewLine();
   void AddToPost(const std::string&, const std::string&);
   void AddToPost(const std::string&, int);
   void AddToPost(const std::string&, double);
   void SendPost();
};

#endif

#endif

//Created by KVClassFactory on Wed May 20 11:31:27 2015
//Author: John Frankland,,,

#include "FzHttpPost.h"
#include <iostream>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>FZHttpPost</h2>
<h4>Class using libcurl to send HTTP POST data to a server</h4>
<!-- */
// --> END_HTML
//
// If we want to add the following lines to the database table:
//
//  row | A |   B  |  C
//  ---------------------
//   1  | 6 | toto | 1.34
//   2  | 3 | tata | 0.54
//
// and if the PHP array name is 'par',
// the PHP array sent to the server should look like:
//
//   par[0][A] = '6'
//   par[0][B] = 'toto'
//   par[0][C] = '1.34'
//   par[1][A] = '3'
//   par[1][B] = 'tata'
//   par[1][C] = '0.54'
//
// which can be achieved by doing:
//
//   SetServerURL("http://localhost/add_entry.php");
//   SetPHPArrayName("par");
//   NewPost();
//   AddToPost("A","6");
//   AddToPost("B","toto");
//   AddToPost("C","1.34");
//   NewLine();
//   AddToPost("A","3");
//   AddToPost("B","tata");
//   AddToPost("C","0.54");
//   SendPost();
////////////////////////////////////////////////////////////////////////////////

// default maximum number of key-value pairs in one PHP POST
int FZHttpPost::fMaxPHP = 1000;

// number of key-value pairs added to each POST by default
// it is currently 2: corresponding to the 'user' and 'category' fields
int FZHttpPost::fNHdrElements = 2;

/* static */ size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
   return size *nmemb;
}

void FZHttpPost::AddKeyValuePair(const std::string& key, const std::string& value)
{
   // Add a key-value pair to the POST

   curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, key.data(),
                CURLFORM_COPYCONTENTS, value.data(),
                CURLFORM_END);
   // increment number of key-value pairs in post
   ++fPostPairs;
}

FZHttpPost::FZHttpPost()
{
   // Default constructor

   fDBUserName = "SlowControls";
   fDBCategory = "SlowControls";

   /* initialise CURL stuff */
   curl_global_init(CURL_GLOBAL_ALL);
   fCURL = curl_easy_init();
   curl_easy_setopt(fCURL, CURLOPT_WRITEFUNCTION, write_data);

   // disable console output
   curl_easy_setopt(fCURL, CURLOPT_VERBOSE, 0L);
   formpost=lastptr=NULL;
}

FZHttpPost::~FZHttpPost()
{
   // Destructor

   /* cleanup CURL stuff */
   curl_easy_cleanup(fCURL);
}

void FZHttpPost::SetDBUserName(const std::string& a)
{
   // Set name of user for FAZIALOG database
   // Will be sent with other POST data as a 'user' field

   fDBUserName=a;
}

void FZHttpPost::SetDBCategory(const std::string& a)
{
   // Set name of category for FAZIALOG database
   // Will be sent with other POST data as a 'category' field

   fDBCategory=a;
}

void FZHttpPost::SetPHPLimit(int max)
{
   // Static method: can be used to change default maximum number of key-value pairs in one PHP POST
   fMaxPHP = max;
}

void FZHttpPost::SetPHPArrayName(const std::string& a)
{
   // Set the name to be used for the PHP array which will be constructed
   // and sent using the POST.
   //
   // e.g. if in the receiving PHP code you look for an array called 'par':
   //    $par = $_POST['par'];
   // then you should do
   //    SetPHPArrayName("par");

   fPHPArrayName = a;
   fPHPArrayFormatString = std::string(a.data()) + "[%d][%s]";
   //fPHPArrayFormatString.Form("%s[%%d][%%s]",a.data());
}

void FZHttpPost::SetServerURL(const std::string& u)
{
   // Set the full URL to which the HTTP POST should be sent
   //
   //  e.g. SetServerURL("http://localhost/add_entry.php");

   fServerURL = u;
   curl_easy_setopt(fCURL, CURLOPT_URL, u.data());
}

const char* FZHttpPost::GetServerURL() const
{
   // Returns full URL to which the HTTP POST will be sent

   return fServerURL.data();
}

void FZHttpPost::NewPost()
{
   // Begin a new posting to the database server, i.e. a set of lines
   // to add like in this example:
   //
   //  row | A |   B  |  C
   //  ---------------------
   //   1  | 6 | toto | 1.34
   //   2  | 3 | tata | 0.54

   // reset curl FORMADD pointers
   formpost=lastptr=NULL;
   // reset line number (this is also the first array index, i.e. 'par[fLineNumber][some-key]')
   fLineNumber=0;
   // reset number of key-value pairs per line - will be set on first call to NewLine()
   fPairsPerLine=0;
   // reset total number of key-value pairs in post
   fPostPairs=0;
}

void FZHttpPost::NewLine()
{
   // Call this method before the second & all subsequent lines of a multi-line
   // post to the database, e.g.
   //
   //   NewPost();
   //   AddToPost("A","6");
   //   AddToPost("B","toto");
   //   AddToPost("C","1.34");
   //   NewLine();  <------ begin a new line
   //   AddToPost("A","3");
   //   AddToPost("B","tata");
   //   AddToPost("C","0.54");
   //
   // If adding a new line would exceed the total number of key-value pairs
   // that PHP can accept, we automatically send everything we have before
   // starting a new line

   if(!fPostPairs){
      // AddToPost has not been called, do nothing (i.e. if we call
      // NewLine() just after NewPost() by mistake)
      return;
   }
   // take number of key-value pairs per line from structure of first line
   if(!fPairsPerLine) fPairsPerLine=fPostPairs;
   // automatically send everything we have before PHP limit reached
   if(CheckPHPLimit())
      SendPost();
   else
      ++fLineNumber;
}

void FZHttpPost::AddToPost(const std::string& p, const std::string& v)
{
   // Add a 'parameter=value' pair to the current POST array member
   // i.e. we effectively define the following member of the PHP array:
   //
   //    fPHPArrayName[fLineNumber][p] = v
   //
   // Example:
   //    SetPHPArrayName("par");
   //    NewPost();
   //    AddToPost("toto","tata");
   // --> par[0][toto]='tata'

   char supp[50];
   std::string array_element;
   
   sprintf(supp, fPHPArrayFormatString.data(), fLineNumber, p.data()); 
   //array_element.Form(fPHPArrayFormatString.data(), fLineNumber, p.data());

   array_element = supp;

   AddKeyValuePair(array_element,v);
}

void FZHttpPost::AddToPost(const std::string& p, int v)
{
   // Add a 'parameter=value' pair to the current POST array member
   // i.e. we effectively define the following member of the PHP array:
   //
   //    fPHPArrayName[fLineNumber][p] = v
   //
   // Example:
   //    SetPHPArrayName("par");
   //    NewPost();
   //    AddToPost("toto",234);
   // --> par[0][toto]='234'

   std::stringstream ss;

   ss << v;
   AddToPost(p,ss.str());
}

void FZHttpPost::AddToPost(const std::string& p, double v)
{
   // Add a 'parameter=value' pair to the current POST array member
   // i.e. we effectively define the following member of the PHP array:
   //
   //    fPHPArrayName[fLineNumber][p] = v
   //
   // Example:
   //    SetPHPArrayName("par");
   //    NewPost();
   //    AddToPost("toto",1.234);
   // --> par[0][toto]='1.234'

   std::stringstream ss;

   ss << v;
   AddToPost(p,ss.str());
}

void FZHttpPost::SendPost()
{
   // Send current array contents to server
   // The 'user' (=fDBUserName) and 'category' (=fDBCategory) fields are added to the POST

   if(!fPostPairs) return;// nothing to send

   // add the 2 'header' elements to the POST (=> corresponds to value of fNHdrElements)
   // N.B. if the number of these elements changes you should modify fNHdrElements!
   AddKeyValuePair("user",fDBUserName);
   AddKeyValuePair("category",fDBCategory);

   //std::cout << "Sending " << fLineNumber+1 << "lines (" << fPostPairs << " key-value pairs) to server" << std::endl;

   curl_easy_setopt(fCURL, CURLOPT_HTTPPOST, formpost);
   /* CURLcode res = */ curl_easy_perform(fCURL);
/*
   if (res != CURLE_OK)
      std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res);
*/
   curl_formfree(formpost);
   formpost = lastptr = NULL;

   // reset line number (this is also the first array index, i.e. 'par[fLineNumber][some-key]')
   fLineNumber=0;
   // reset total number of key-value pairs in post
   fPostPairs=0;
}


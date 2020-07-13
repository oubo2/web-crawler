# web-crawler
A simple web crawler in c that takes an initial command line argument with a valid url with prefix "http://", then recursively fetch all url on the page, until 100 urls are fetched or no more urls can be found. No pages that are the same are fetched twiced(that may have different url). At the end, print a log of all urls that are fetched. 
It does not use any existing HTTP library, basics is connecting socket from client with host(different websites), and parsing urls from response.
Return code handling is not finalized.

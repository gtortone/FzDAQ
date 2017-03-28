#include "FzController.h"

bool FzController::map_insert(std::string const &k, Report::Node const &v) {

   mapmutex.lock();

   std::pair<std::map<std::string, Report::Node>::iterator, bool> result = map.insert(std::make_pair(k, v));

   if (result.second) { // element does not exist in map - insert in deque

      deque.push_back(std::make_pair(result.first, time(NULL)));

   } else {     // element exists in map - update element in map and update time in deque

      map[k] = v;

      for(uint8_t i=0; i<deque.size(); i++) {

         if(deque[i].first->first == result.first->first)
            deque[i].second = time(NULL);
      }
   }

   mapmutex.unlock();

   return result.second;
}

void FzController::map_clean(void) {

   mapmutex.lock();

   for(uint8_t i=0; i<deque.size(); i++) {

      if(difftime(time(NULL), deque[i].second) > EXPIRY) {

         map.erase(deque[i].first);
         deque.erase(deque.begin() + i);
      }
   }

   mapmutex.unlock();
}


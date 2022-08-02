#pragma once

#include <iostream>
#include <string>
#include <set>
#include <execution>

#include "search_server.h"


void RemoveDuplicates(SearchServer& search_server);

void RemoveDuplicates(std::execution::sequenced_policy, SearchServer& search_server);

void RemoveDuplicates(std::execution::parallel_policy policy, SearchServer& search_server);
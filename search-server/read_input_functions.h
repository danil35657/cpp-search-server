#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "document.h"

using std::literals::string_literals::operator""s;

std::string ReadLine();

int ReadLineWithNumber();


std::ostream& operator<<(std::ostream& output, Document document);
    
template <typename Element> 
std::ostream& operator<<(std::ostream& output, std::vector<Element> v) {
    for (auto document : v) {
        output << document;
    }
return output;
}
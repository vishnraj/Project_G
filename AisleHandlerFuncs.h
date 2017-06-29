#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory> 

// Parse funcs

// Store data parsing

void parse_store_request(const std::string & buffer
    , std::shared_ptr<std::unordered_map<std::string, std::string> > params_to_vals
    , std::string & response);

// Aisle data parsing

void parse_aisle_request(const std::string & buffer
    , std::shared_ptr<std::unordered_map<std::string, std::string> > params_to_vals
    , std::string & response);

// Validation funcs

// Store data validation

bool verify_store_request(
    std::shared_ptr<const std::unordered_map<std::string
        , std::string> > params_to_vals
    , const std::unordered_map<std::string, std::pair<int, int> > *& cols
    , const std::vector<std::string> *& keys
    , std::string & table_name, std::string & response);

// Aisle data validation

bool verify_aisle_request(
    std::shared_ptr<std::vector<std::unordered_map<std::string
        , std::string> > > data
    , std::shared_ptr<const std::unordered_map<std::string
        , std::string> > params_to_vals
    , const std::unordered_map<std::string, std::pair<int, int> > *& cols
    , std::string & table_name, std::string & response);

// Select funcs

void select_store_data(const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , std::string & response);

void select_store_aisles(const std::string & query
    , const std::vector<std::pair<int, int> > & col_meta_data
    , std::string & response);

// Insert funcs

void insert_aisle_data(
    const std::vector<std::string> & insert_queries
    , std::string &);

#ifndef CLP_DATABASE_UTILS_HPP
#define CLP_DATABASE_UTILS_HPP

#include <string>
#include <vector>

namespace clp {
/**
 * Gets the SQL for a list of field names and types in the form
 * "field_name1 TYPE1,field_name2 TYPE2,..."
 * @param field_names_and_types
 * @return The SQL
 */
std::string get_field_names_and_types_sql(
        std::vector<std::pair<std::string, std::string>> const& field_names_and_types
);
/**
 * Gets the SQL for a list of field names in the form "field_name1,field_name2,..."
 * @param field_names_and_types
 * @return The SQL
 */
std::string get_field_names_sql(
        std::vector<std::pair<std::string, std::string>> const& field_names_and_types
);
/**
 * Gets the SQL for a list of field names in the form "field_name1,field_name2,..."
 * @param field_names
 * @return The SQL
 */
std::string get_field_names_sql(std::vector<std::string> const& field_names);

/**
 * Gets the SQL for the given number of placeholders
 * @param num_placeholders
 * @return The SQL
 */
std::string get_placeholders_sql(size_t num_placeholders);
/**
 * Gets the SQL for the given number of numbered placeholders
 * @param num_placeholders
 * @return The SQL
 */
std::string get_numbered_placeholders_sql(size_t num_placeholders);

/**
 * Gets the SQL to set a list of fields to placeholders in the form
 * "field_name1 = ?,field_name2 = ?,..."
 * @param field_names
 * @param begin_ix Which field to start from
 * @return The SQL
 */
std::string
get_set_field_sql(std::vector<std::string> const& field_names, size_t begin_ix, size_t end_ix);
/**
 * Gets the SQL to set a list of fields to numbered placeholders in the form
 * "field_name1 = ?1,field_name2 = ?2,..."
 * @param field_names_and_types
 * @param begin_ix Which field to start from
 * @return The SQL
 */
std::string get_numbered_set_field_sql(
        std::vector<std::pair<std::string, std::string>> const& field_names_and_types,
        size_t begin_ix
);
/**
 * Gets the SQL to set a list of fields to numbered placeholders in the form
 * "field_name1 = ?1,field_name2 = ?2,..."
 * @param field_names
 * @param begin_ix Which field to start from
 * @return The SQL
 */
std::string
get_numbered_set_field_sql(std::vector<std::string> const& field_names, size_t begin_ix);
}  // namespace clp

#endif  // CLP_DATABASE_UTILS_HPP

/**
 * @file
 * @brief Collection of all configuration exceptions
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_CONFIG_EXCEPTIONS_H
#define ALLPIX_CONFIG_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Base class for all configurations exceptions in the framework.
     */
    class ConfigurationError : public Exception {};

    /**
     * @ingroup Exceptions
     * @brief Notifies of a missing configuration file
     */
    // TODO [doc] Rename to not found?
    class ConfigFileUnavailableError : public ConfigurationError {
    public:
        /**
         * @brief Construct an error for a configuration that is not found
         * @param file_name Name of the configuration file
         */
        explicit ConfigFileUnavailableError(const std::string& file_name) {
            error_message_ = "Could not read configuration file " + file_name + " (does it exists?)";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Indicates a problem converting the value of a configuration key to the value it should represent
     */
    // TODO [doc] this should be InvalidValueTypeError (see below)
    class InvalidKeyError : public ConfigurationError {
    public:
        /**
         * @brief Construct an error for a value with an invalid type
         * @param key Name of the corresponding key
         * @param section Section where the key/value pair was defined
         * @param value Value as a literal string
         * @param type Type where the value should have been converted to
         * @param reason Reason why the conversion failed
         */
        InvalidKeyError(const std::string& key,
                        const std::string& section,
                        const std::string& value,
                        const std::type_info& type,
                        const std::string& reason) {
            // FIXME: file and line number are missing
            std::string section_str = "in section '" + section + "'";
            if(section.empty()) {
                section_str = "in empty section";
            }

            error_message_ = "Could not convert value '" + value + "' of key '" + key + "' " + section_str + " to type " +
                             allpix::demangle(type.name());
            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Informs of a missing key that should have been defined
     */
    class MissingKeyError : public ConfigurationError {
    public:
        /**
         * @brief Construct an error for a missing key
         * @param key Name of the missing key
         * @param section Section where the key should have been defined
         */
        MissingKeyError(const std::string& key, const std::string& section) {
            // FIXME: file and line number are missing
            std::string section_str = "in section '" + section + "'";
            if(section.empty()) {
                section_str = "in empty section";
            }
            error_message_ = "Key '" + key + "' " + section_str + " does not exist";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Indicates an error while parsing a configuration file
     */
    // TODO [doc] Rename to ConfigurationError
    class ConfigParseError : public ConfigurationError {
    public:
        /**
         * @brief Construct an error for a missing key
         * @param file_name Name of the configuration file
         * @param line_num Line number where the problem occurred
         */
        ConfigParseError(const std::string& file_name, int line_num) {
            error_message_ = "Could not parse line ";
            error_message_ += std::to_string(line_num);
            error_message_ += " in file '" + file_name + "'";
            error_message_ += ": not a section header, key/value pair or comment";
        }
    };

    class Configuration;
    /**
     * @ingroup Exceptions
     * @brief Indicates an error with the contents of value
     * @note Only configuration error that should be called directly from modules
     *
     * Should be raised if the data contains valid data for its type (otherwise an \ref InvalidKeyError should have been
     * raised earlier), but the value is not in the range of allowed values.
     */
    class InvalidValueError : public ConfigurationError {
    public:
        /**
         * @brief Construct an error for an invalid value
         * @param config Configuration object containing the problematic key
         * @param key Name of the problematic key
         * @param reason Reason why the value is invalid (empty if no explicit reason)
         */
        InvalidValueError(const Configuration& config, const std::string& key, const std::string& reason = "");
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_EXCEPTIONS_H */

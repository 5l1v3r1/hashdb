// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Header file for the hashdb library.
 */

#ifndef HASHDB_HPP
#define HASHDB_HPP

#include <string>
#include <set>
#include <stdint.h>
#include <sys/time.h>   // timeval for timestamp

// ************************************************************
// version of the hashdb library
// ************************************************************
/**
 * Version of the hashdb library, same as hashdb::version.
 */
extern "C"
const char* hashdb_version();

namespace hashdb {
  class lmdb_hash_data_manager_t;
  class lmdb_hash_manager_t;
  class lmdb_source_data_manager_t;
  class lmdb_source_id_manager_t;
  class lmdb_source_name_manager_t;
  class lmdb_changes_t;
  class logger_t;

  // ************************************************************
  // typedefs
  // ************************************************************
  // pair(file_binary_hash, file_offset)
  typedef std::pair<std::string, uint64_t>    source_offset_pair_t;
  typedef std::set<source_offset_pair_t> source_offset_pairs_t;

  // pair(repository_name, filename)
  typedef std::pair<std::string, std::string> source_name_t;
  typedef std::set<source_name_t>             source_names_t;

  // ************************************************************
  // version of the hashdb library
  // ************************************************************
  /**
   * Version of the hashdb library.
   */
  extern "C"
  const char* version();

  // ************************************************************
  // settings
  // ************************************************************
  /**
   * Provides hashdb settings.
   *
   * Attributes:
   *   settings_version - The version of the settings record
   *   sector_size - Minimal sector size of data, in bytes.  Blocks must
   *     align to this.
   *   block_size - Size, in bytes, of data blocks.
   *   max_source_offset_pairs - The maximum number of source hash,
   *     file offset pairs to store for a single hash value.
   *   hash_prefix_bits - The number of hash prefix bits to use as the
   *     key in the optimized hash storage.
   *   hash_suffix_bytes - The number of hash suffix bytes to use as the
   *     value in the optimized hash storage.
   */
  struct settings_t {
    static const uint32_t CURRENT_SETTINGS_VERSION = 3;
    uint32_t settings_version;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t max_source_offset_pairs;
    uint32_t hash_prefix_bits;
    uint32_t hash_suffix_bytes;
    settings_t();
    std::string settings_string() const;
  };

  // ************************************************************
  // misc support interfaces
  // ************************************************************
  /**
   * Create a new hashdb.
   * Return true and "" if hashdb is created, false and reason if not.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to create.  The path must not
   *     exist yet.
   *   settings - The hashdb settings.
   *   command_string - String to put into the new hashdb log.
   *
   * Returns tuple:
   *   True and "" if creation was successful, false and reason if not.
   */
  std::pair<bool, std::string> create_hashdb(
                     const std::string& hashdb_dir,
                     const hashdb::settings_t& settings,
                     const std::string& command_string);

  /**
   * Return hashdb settings else false and reason.
   * The current implementation may abort if something worse than a simple
   * path problem happens.
   *
   * Parameters:
   *   hashdb_dir - Path to the database to obtain the settings of.
   *   settings - The hashdb settings.
   *
   * Returns tuple:
   *   True and "" if settings were retrieved, false and reason if not.
   */
  std::pair<bool, std::string> read_settings(
                     const std::string& hashdb_dir,
                     hashdb::settings_t& settings);

  /**
   * Print environment information to the stream.  Specifically, print
   * lines starting with the pound character followed by version information,
   * the command line, the username, if available, and the date.
   */
  void print_environment(const std::string& command_line, std::ostream& os);

  // ************************************************************
  // import
  // ************************************************************
  /**
   * Manage all LMDB updates.  All interfaces are locked and threadsafe.
   * A logger is opened for logging the command and for logging
   * timestamps and changes applied during the session.  Upon closure,
   * changes are written to the logger and the logger is closed.
   */
  class import_manager_t {

    private:
    lmdb_hash_data_manager_t* lmdb_hash_data_manager;
    lmdb_hash_manager_t* lmdb_hash_manager;
    lmdb_source_data_manager_t* lmdb_source_data_manager;
    lmdb_source_id_manager_t* lmdb_source_id_manager;
    lmdb_source_name_manager_t* lmdb_source_name_manager;

    logger_t* logger;
    hashdb::lmdb_changes_t* changes;

    public:
    // do not allow copy or assignment
#ifdef HAVE_CXX11
    import_manager_t(const import_manager_t&) = delete;
    import_manager_t& operator=(const import_manager_t&) = delete;
#else
    import_manager_t(const import_manager_t&) __attribute__ ((noreturn));
    import_manager_t& operator=(const import_manager_t&)
                                              __attribute__ ((noreturn));
#endif

    /**
     * Open hashdb for importing.
     *
     * Parameters:
     *   hashdb_dir - Path to the hashdb data store to import into.
     */
    import_manager_t(const std::string& hashdb_dir,
                     const std::string& command_string);

    /**
     * The destructor closes the log file and data store resources.
     */
    ~import_manager_t();

    /**
     * Insert the repository_name, filename pair associated with the
     * source.
     */
    void insert_source_name(const std::string& file_binary_hash,
                            const std::string& repository_name,
                            const std::string& filename);


    /**
     * Insert or change source data.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   filesize - The size of the source, in bytes.
     *   file_type - A string representing the type of the file.
     *   nonprobative_count - The count of non-probative hashes
     *     identified for this source.
     */
    void insert_source_data(const std::string& file_binary_hash,
                            const uint64_t filesize,
                            const std::string& file_type,
                            const uint64_t nonprobative_count);

    /**
     * Insert or change the hash data associated with the binary_hash.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   file_offset - The byte offset into the file where the hash is
     *     located.
     *   entropy - A numeric entropy value for the associated block.
     *   block_label - Text indicating the type of the block or "" for
     *     no label.
     */
    void insert_hash(const std::string& binary_hash,
                     const std::string& file_binary_hash,
                     const uint64_t file_offset,
                     const uint64_t entropy,
                     const std::string& block_label);

    /**
     * Returns sizes of LMDB databases in the data store.
     */
    std::string sizes() const;
  };

  // ************************************************************
  // scan
  // ************************************************************
  /**
   * Manage LMDB scans.  Interfaces should be threadsafe by LMDB design.
   */
  class scan_manager_t {

    private:
    lmdb_hash_data_manager_t* lmdb_hash_data_manager;
    lmdb_hash_manager_t* lmdb_hash_manager;
    lmdb_source_data_manager_t* lmdb_source_data_manager;
    lmdb_source_id_manager_t* lmdb_source_id_manager;
    lmdb_source_name_manager_t* lmdb_source_name_manager;

    // support scan_expanded
    std::set<std::string>* hashes;
    std::set<std::string>* sources;

    public:
    // do not allow copy or assignment
#ifdef HAVE_CXX11
    scan_manager_t(const scan_manager_t&) = delete;
    scan_manager_t& operator=(const scan_manager_t&) = delete;
#else
    scan_manager_t(const scan_manager_t&) __attribute__ ((noreturn));
    scan_manager_t& operator=(const scan_manager_t&)
                                          __attribute__ ((noreturn));
#endif

    /**
     * Open hashdb for scanning.
     *
     * Parameters:
     *   hashdb_dir - Path to the database to scan against.
     */
    scan_manager_t(const std::string& hashdb_dir);

    /**
     * The destructor closes read-only data store resources.
     */
    ~scan_manager_t();

    /**
     * Scan for a hash and return expanded source information associated
     * with it.
     *
     * scan_manager caches hashes and source IDs and does not return
     * source information for hashes or sources that has already been
     * returned.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form to scan for.
     *   expanded_text - Text about matched sources, or blank if text
     *     for the scanned hash has been returned in a previous scan.
     *
     *     Text is in JSON format.  Example syntax:
     * 
     * {
     *   "entropy": 8,
     *   "block_label": "W",
     *   "source_list_id": 57,
     *   "sources": [{
     *     "file_hash": "f7035a...",
     *     "filesize": 800,
     *     "file_type": "exe",
     *     "nonprobative_count": 2,
     *     "names": ["repository1", "filename1", "repo2", "f2"]
     *   }],
     *   "source_offset_pairs": ["f7035a...", 0, "f7035a...", 512]
     * }
     *
     * Returns:
     *   True if the hash is present, false if not.
     */
    bool find_expanded_hash(const std::string& binary_hash,
                            std::string& expanded_text);

    /**
     * Find hash.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *   entropy - A numeric entropy value for the associated block.
     *   block_label - Text indicating the type of the block or "" for
     *     no label.
     *   source_offset_pairs - Set of pairs of source hash and file
     *     offset values.
     *
     * Returns:
     *   True if the hash is present, false if not.
     */
    bool find_hash(const std::string& binary_hash,
                   uint64_t& entropy,
                   std::string& block_label,
                   source_offset_pairs_t& source_offset_pairs) const;

    /**
     * Find hash count.  Faster than find_hash.  Accesses the hash
     * information store.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   Approximate hash count.
     */
    size_t find_hash_count(const std::string& binary_hash) const;

    /**
     * Find approximate hash count.  Faster than find_hash, but can be wrong.
     * Accesses the hash store.
     *
     * Parameters:
     *   binary_hash - The block hash in binary form.
     *
     * Returns:
     *   Approximate hash count.
     */
    size_t find_approximate_hash_count(const std::string& binary_hash) const;

    /**
     * Find source data for the given source ID, false on no source ID.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   filesize - The size of the source, in bytes.
     *   file_type - A string representing the type of the file.
     *   nonprobative_count - The count of non-probative hashes
     *     identified for this source.
     */
    bool find_source_data(const std::string& file_binary_hash,
                          uint64_t& filesize,
                          std::string& file_type,
                          uint64_t& nonprobative_count) const;

    /**
     * Find source names for the given source ID, false on no source ID.
     *
     * Parameters:
     *   file_binary_hash - The MD5 hash of the source in binary form.
     *   source_names - Set of pairs of repository_name, filename
     *     attributed to this source ID.
     */
    bool find_source_names(const std::string& file_binary_hash,
                           source_names_t& source_names) const;

    /**
     * Return the first block hash in the database.
     *
     * Returns pair:
     *   True if a first hash is available, false and "" if DB is empty.
     */
    std::pair<bool, std::string> hash_begin() const;

    /**
     * Return the next block hash in the database.  Error if last hash
     *   does not exist.
     *
     * Returns pair:
     *   True if hash is available, false and "" if at end of DB.
     */
    std::pair<bool, std::string> hash_next(
                          const std::string& last_binary_hash) const;


    /**
     * Return the first source in the database.
     *
     * Returns pair:
     *   True if source is available, false and "" if DB is empty.
     */
    std::pair<bool, std::string> source_begin() const;

    /**
     * Return the next source in the database.  Error if last_file_binary_hash
     *   does not exist.
     *
     * Returns pair:
     *   True if source is available, false and "" if at end of DB.
     */
    std::pair<bool, std::string> source_next(
                            const std::string& last_file_binary_hash) const;

    /**
     * Return sizes of LMDB databases in JSON format.
     */
    std::string sizes() const;

    /**
     * Return the number of unique hashes in the database.
     */
    size_t size() const;
  };

  // ************************************************************
  // timestamp
  // ************************************************************
  /**
   * Provide a timestamp service.
   */
  class timestamp_t {

    private:
    struct timeval* t0;
    struct timeval* t_last_timestamp;

    public:

    /**
     * Create a timestamp service.
     */
    timestamp_t();

    /**
     * Release timestamp resources.
     */
    ~timestamp_t();

    // do not allow copy or assignment
#ifdef HAVE_CXX11
    timestamp_t(const timestamp_t&) = delete;
    timestamp_t& operator=(const timestamp_t&) = delete;
#else
    timestamp_t(const timestamp_t&) __attribute__ ((noreturn));
    timestamp_t& operator=(const timestamp_t&) __attribute__ ((noreturn));
#endif

    /**
     * Create a named timestamp and return a JSON string in format
     * {"name":"name", "delta":delta, "total":total}.
     */
    std::string stamp(const std::string &name);
  };
}

#endif


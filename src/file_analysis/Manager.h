// See the file "COPYING" in the main distribution directory for copyright.

#ifndef FILE_ANALYSIS_MANAGER_H
#define FILE_ANALYSIS_MANAGER_H

#include <string>
#include <map>
#include <set>
#include <queue>

#include "Net.h"
#include "Conn.h"
#include "Val.h"
#include "Analyzer.h"
#include "Timer.h"
#include "EventHandler.h"

#include "File.h"
#include "FileTimer.h"
#include "Component.h"

#include "analyzer/Tag.h"

#include "file_analysis/file_analysis.bif.h"

namespace file_analysis {

/**
 * Main entry point for interacting with file analysis.
 */
class Manager {
public:

	/**
	 * Constructor.
	 */
	Manager();

	/**
	 * Destructor.  Times out any currently active file analyses.
	 */
	~Manager();

	/**
	 * First-stage initializion of the manager. This is called early on
	 * during Bro's initialization, before any scripts are processed.
	 */
	void InitPreScript();

	/**
	 * Second-stage initialization of the manager. This is called late
	 * during Bro's initialization after any scripts are processed.
	 */
	void InitPostScript();

	/**
	 * Times out any active file analysis to prepare for shutdown.
	 */
	void Terminate();

	/**
	 * Creates a file identifier from a unique file handle string.
	 * @param handle a unique string which identifies a single file.
	 * @return a prettified MD5 hash of \a handle, truncated to 64-bits.
	 */
	string HashHandle(const string& handle) const;

	/**
	 * Take in a unique file handle string to identify next piece of
	 * incoming file data/information.
	 * @param handle a unique string which identifies a single file.
	 */
	void SetHandle(const string& handle);

	/**
	 * Pass in non-sequential file data.
	 * @param data pointer to start of a chunk of file data.
	 * @param len number of bytes in the data chunk.
	 * @param offset number of bytes from start of file that data chunk occurs.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction.
	 */
	void DataIn(const u_char* data, uint64 len, uint64 offset,
		    analyzer::Tag tag, Connection* conn, bool is_orig);

	/**
	 * Pass in sequential file data.
	 * @param data pointer to start of a chunk of file data.
	 * @param len number of bytes in the data chunk.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction.
	 */
	void DataIn(const u_char* data, uint64 len, analyzer::Tag tag,
	            Connection* conn, bool is_orig);

	/**
	 * Pass in sequential file data from external source (e.g. input framework).
	 * @param data pointer to start of a chunk of file data.
	 * @param len number of bytes in the data chunk.
	 * @param file_id an identifier for the file (usually a hash of \a source).
	 * @param source uniquely identifies the file and should also describe
	 *        in human-readable form where the file input is coming from (e.g.
	 *        a local file path).
	 */
	void DataIn(const u_char* data, uint64 len, const string& file_id,
	            const string& source);

	/**
	 * Signal the end of file data regardless of which direction it is being
	 * sent over the connection.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 */
	void EndOfFile(analyzer::Tag tag, Connection* conn);

	/**
	 * Signal the end of file data being transferred over a connection in
	 * a particular direction.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 */
	void EndOfFile(analyzer::Tag tag, Connection* conn, bool is_orig);

	/**
	 * Signal the end of file data being transferred using the file identifier.
	 * @param file_id the file identifier/hash.
	 */
	void EndOfFile(const string& file_id);

	/**
	 * Signal a gap in the file data stream.
	 * @param offset number of bytes in to file at which missing chunk starts.
	 * @param len length in bytes of the missing chunk of file data.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction.
	 */
	void Gap(uint64 offset, uint64 len, analyzer::Tag tag, Connection* conn,
	         bool is_orig);

	/**
	 * Provide the expected number of bytes that comprise a file.
	 * @param size the number of bytes in the full file.
	 * @param tag network protocol over which the file data is transferred.
	 * @param conn network connection over which the file data is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction.
	 */
	void SetSize(uint64 size, analyzer::Tag tag, Connection* conn,
	             bool is_orig);

	/**
	 * Starts ignoring a file, which will finally be removed from internal
	 * mappings on EOF or TIMEOUT.
	 * @param file_id the file identifier/hash.
	 * @return false if file identifier did not map to anything, else true.
	 */
	bool IgnoreFile(const string& file_id);

	/**
	 * Set's an inactivity threshold for the file.
	 * @param file_id the file identifier/hash.
	 * @param interval the amount of time in which no activity is seen for
	 *        the file identified by \a file_id that will cause the file
	 *        to be considered stale, timed out, and then resource reclaimed.
	 * @return false if file identifier did not map to anything, else true.
	 */
	bool SetTimeoutInterval(const string& file_id, double interval) const;

	/**
	 * Queue attachment of an analzer to the file identifier.  Multiple
	 * analyzers of a given type can be attached per file identifier at a time
	 * as long as the arguments differ.
	 * @param file_id the file identifier/hash.
	 * @param args a \c AnalyzerArgs value which describes a file analyzer.
	 * @return false if the analyzer failed to be instantiated, else true.
	 */
	bool AddAnalyzer(const string& file_id, RecordVal* args) const;

	/**
	 * Queue removal of an analyzer for a given file identifier.
	 * @param file_id the file identifier/hash.
	 * @param args a \c AnalyzerArgs value which describes a file analyzer.
	 * @return true if the analyzer is active at the time of call, else false.
	 */
	bool RemoveAnalyzer(const string& file_id, const RecordVal* args) const;

	/**
	 * Tells whether analysis for a file is active or ignored.
	 * @param file_id the file identifier/hash.
	 * @return whether the file mapped to \a file_id is being ignored.
	 */
	bool IsIgnored(const string& file_id);

	/**
	 * Instantiates a new file analyzer instance for the file.
	 * @param tag The file analyzer's tag.
	 * @param args The file analzer argument/option values.
	 * @param f The file analzer is to be associated with.
	 * @return The new analyzer instance or null if tag is invalid.
	 */
	Analyzer* InstantiateAnalyzer(int tag, RecordVal* args, File* f) const;

	/**
	 * Translates a script-level file analyzer tag in to corresponding file
	 * analyzer name.
	 * @param tag The enum val of a file analyzer.
	 * @return The human-readable name of the file analyzer.
	 */
	const char* GetAnalyzerName(int tag) const;

protected:
	friend class FileTimer;

	typedef set<string> IDSet;
	typedef map<string, File*> IDMap;

	/**
	 * Create a new file to be analyzed or retrieve an existing one.
	 * @param file_id the file identifier/hash.
	 * @param conn network connection, if any, over which the file is
	 *        transferred.
	 * @param tag network protocol, if any, over which the file is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction (or if it
	 *        this file isn't related to a connection).
	 * @param update_conn whether we need to update connection-related field
	 *        in the \c fa_file record value associated with the file.
	 * @return the File object mapped to \a file_id or a null pointer if
	 *         analysis is being ignored for the associated file.  An File
	 *         object may be created if a mapping doesn't exist, and if it did
	 *         exist, the activity time is refreshed along with any
	 *         connection-related fields.
	 */
	File* GetFile(const string& file_id, Connection* conn = 0,
	              analyzer::Tag tag = analyzer::Tag::Error,
	              bool is_orig = false, bool update_conn = true);

	/**
	 * Try to retrieve a file that's being analyzed, using its identifier/hash.
	 * @param file_id the file identifier/hash.
	 * @return the File object mapped to \a file_id, or a null pointer if no
	 *         mapping exists.
	 */
	File* Lookup(const string& file_id) const;

	/**
	 * Evaluate timeout policy for a file and remove the File object mapped to
	 * \a file_id if needed.
	 * @param file_id the file identifier/hash.
	 * @param is_termination whether the Manager (and probably Bro) is in a
	 *        terminating state.  If true, then the timeout cannot be postponed.
	 */
	void Timeout(const string& file_id, bool is_terminating = ::terminating);

	/**
	 * Immediately remove file_analysis::File object associated with \a file_id.
	 * @param file_id the file identifier/hash.
	 * @return false if file id string did not map to anything, else true.
	 */
	bool RemoveFile(const string& file_id);

	/**
	 * Sets #current_file_id to a hash of a unique file handle string based on
	 * what the \c get_file_handle event derives from the connection params.
	 * Event queue is flushed so that we can get the handle value immediately.
	 * @param tag network protocol over which the file is transferred.
	 * @param conn network connection over which the file is transferred.
	 * @param is_orig true if the file is being sent from connection originator
	 *        or false if is being sent in the opposite direction.
	 */
	void GetFileHandle(analyzer::Tag tag, Connection* c, bool is_orig);

	/**
	 * Check if analysis is available for files transferred over a given
	 * network protocol.
	 * @param tag the network protocol over which files can be transferred and
	 *        analyzed by the file analysis framework.
	 * @return whether file analysis is disabled for the analyzer given by
	 *         \a tag.
	 */
	static bool IsDisabled(analyzer::Tag tag);

private:
	typedef map<string, Component*> analyzer_map_by_name;
	typedef map<analyzer::Tag, Component*> analyzer_map_by_tag;
	typedef map<int, Component*> analyzer_map_by_val;

	void RegisterAnalyzerComponent(Component* component);

	IDMap id_map;	/**< Map file ID to file_analysis::File records. */
	IDSet ignored;	/**< Ignored files.  Will be finally removed on EOF. */
	string current_file_id;	/**< Hash of what get_file_handle event sets. */
	EnumType* tag_enum_type;	/**< File analyzer tag type. */

	analyzer_map_by_name analyzers_by_name;
	analyzer_map_by_tag analyzers_by_tag;
	analyzer_map_by_val analyzers_by_val;

	static TableVal* disabled;	/**< Table of disabled analyzers. */
	static string salt; /**< A salt added to file handles before hashing. */
};

} // namespace file_analysis

extern file_analysis::Manager* file_mgr;

#endif

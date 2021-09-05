//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name iolib.cpp - Compression-IO helper functions. */
//
//      (c) Copyright 2000-2021 by Andreas Arens, Lutz Sammer Jimmy Salmon,
//                                 Pali Rohár and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.

#include "stratagus.h"

#include "iolib.h"

#include "database/database.h"
#include "game.h"
#include "iocompat.h"
#include "map/map.h"
#include "parameters.h"
//Wyrmgus start
#include "script.h"
//Wyrmgus end
#include "util/util.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

#ifdef __MORPHOS__
#undef tell
#endif

class CFile::PImpl
{
public:
	PImpl();
	~PImpl();

	int open(const std::string &filepath_str, const long flags);
	int close();
	void flush();
	int read(void *buf, size_t len);
	int seek(long offset, int whence);
	long tell();
	int write(const void *buf, size_t len);

private:
	PImpl(const PImpl &rhs); // No implementation
	const PImpl &operator = (const PImpl &rhs); // No implementation

private:
	int   cl_type;   /// type of CFile
	FILE *cl_plain;  /// standard file pointer
#ifdef USE_ZLIB
	gzFile cl_gz;    /// gzip file pointer
#endif // !USE_ZLIB
};

CFile::CFile() : pimpl(std::make_unique<CFile::PImpl>())
{
}

CFile::~CFile()
{
}


/**
**  CLopen Library file open
**
**  @param name       File name.
**  @param openflags  Open read, or write and compression options
**
**  @return File Pointer
*/
int CFile::open(const char *name, long flags)
{
	return pimpl->open(name, flags);
}

/**
**  CLclose Library file close
*/
int CFile::close()
{
	return pimpl->close();
}

void CFile::flush()
{
	pimpl->flush();
}

/**
**  CLread Library file read
**
**  @param buf  Pointer to read the data to.
**  @param len  number of bytes to read.
*/
int CFile::read(void *buf, size_t len)
{
	return pimpl->read(buf, len);
}

/**
**  CLseek Library file seek
**
**  @param offset  Seek position
**  @param whence  How to seek
*/
int CFile::seek(long offset, int whence)
{
	return pimpl->seek(offset, whence);
}

/**
**  CLtell Library file tell
*/
long CFile::tell()
{
	return pimpl->tell();
}

/**
**  CLprintf Library file write
**
**  @param format  String Format.
**  @param ...     Parameter List.
*/
int CFile::printf(const char *format, ...)
{
	int size = 500;
	auto p = std::make_unique<char[]>(size);
	if (p == nullptr) {
		return -1;
	}
	while (1) {
		// Try to print in the allocated space.
		va_list ap;
		va_start(ap, format);
		const int n = vsnprintf(p.get(), size, format, ap);
		va_end(ap);
		// If that worked, string was processed.
		if (n > -1 && n < size) {
			break;
		}
		// Else try again with more space.
		if (n > -1) { // glibc 2.1
			size = n + 1; // precisely what is needed
		} else {    /* glibc 2.0, vc++ */
			size *= 2;  // twice the old size
		}
		p = std::make_unique<char[]>(size);
		if (p == nullptr) {
			return -1;
		}
	}
	size = strlen(p.get());
	int ret = pimpl->write(p.get(), size);
	return ret;
}

//  Implementation.

CFile::PImpl::PImpl()
{
	cl_type = CLF_TYPE_INVALID;
}

CFile::PImpl::~PImpl()
{
	if (cl_type != CLF_TYPE_INVALID) {
		DebugPrint("File wasn't closed\n");
		close();
	}
}

#ifdef USE_ZLIB

#ifndef z_off_t // { ZLIB_VERSION<="1.0.4"

/**
**  Seek on compressed input. (Newer libs support it directly)
**
**  @param file    File
**  @param offset  Seek position
**  @param whence  How to seek
*/
static int gzseek(CFile *file, unsigned offset, int whence)
{
	char buf[32];

	while (offset > sizeof(buf)) {
		gzread(file, buf, sizeof(buf));
		offset -= sizeof(buf);
	}
	return gzread(file, buf, offset);
}

#endif // } ZLIB_VERSION<="1.0.4"

#endif // USE_ZLIB

int CFile::PImpl::open(const std::string &filepath_str, const long openflags)
{
	std::array<char, 512> buf{};

	const char *openstring = nullptr;

	if ((openflags & CL_OPEN_READ) && (openflags & CL_OPEN_WRITE)) {
		openstring = "rwb";
	} else if (openflags & CL_OPEN_READ) {
		openstring = "rb";
	} else if (openflags & CL_OPEN_WRITE) {
		openstring = "wb";
	} else {
		DebugPrint("Bad CLopen flags");
		Assert(0);
		return -1;
	}

	cl_type = CLF_TYPE_INVALID;

	const std::string gz_filepath_str = filepath_str.ends_with(".gz") ? filepath_str : filepath_str + ".gz";

	if (openflags & CL_OPEN_WRITE) {
#ifdef USE_ZLIB
			if ((openflags & CL_WRITE_GZ)
				&& (cl_gz = gzopen(gz_filepath_str.c_str(), openstring))) {
				cl_type = CLF_TYPE_GZIP;
			} else
#endif
				if ((cl_plain = fopen(filepath_str.c_str(), openstring))) {
					cl_type = CLF_TYPE_PLAIN;
				}
	} else {
		if (!(cl_plain = fopen(filepath_str.c_str(), openstring))) { // try plain first
#ifdef USE_ZLIB
			if ((cl_gz = gzopen(gz_filepath_str.c_str(), "rb"))) {
				cl_type = CLF_TYPE_GZIP;
			} else
#endif
				{ }
		} else {
			cl_type = CLF_TYPE_PLAIN;
			// Hmm, plain worked, but nevertheless the file may be compressed!
			if (fread(buf.data(), 2, 1, cl_plain) == 1) {
#ifdef USE_ZLIB
				if (buf[0] == 0x1f) { // don't check for buf[1] == 0x8b, so that old compress also works!
					fclose(cl_plain);
					if ((cl_gz = gzopen(filepath_str.c_str(), "rb"))) {
						cl_type = CLF_TYPE_GZIP;
					} else {
						if (!(cl_plain = fopen(filepath_str.c_str(), "rb"))) {
							cl_type = CLF_TYPE_INVALID;
						}
					}
				}
#endif // USE_ZLIB
			}
			if (cl_type == CLF_TYPE_PLAIN) { // ok, it is not compressed
				rewind(cl_plain);
			}
		}
	}

	if (cl_type == CLF_TYPE_INVALID) {
		//fprintf(stderr, "%s in ", buf);
		return -1;
	}

	return 0;
}

int CFile::PImpl::close()
{
	int ret = EOF;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fclose(cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzclose(cl_gz);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
	cl_type = CLF_TYPE_INVALID;
	return ret;
}

int CFile::PImpl::read(void *buf, size_t len)
{
	int ret = 0;

	if (cl_type != CLF_TYPE_INVALID) {
		if (cl_type == CLF_TYPE_PLAIN) {
			ret = fread(buf, 1, len, cl_plain);
		}
#ifdef USE_ZLIB
		if (cl_type == CLF_TYPE_GZIP) {
			ret = gzread(cl_gz, buf, len);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
	return ret;
}

void CFile::PImpl::flush()
{
	if (cl_type != CLF_TYPE_INVALID) {
		if (cl_type == CLF_TYPE_PLAIN) {
			fflush(cl_plain);
		}
#ifdef USE_ZLIB
		if (cl_type == CLF_TYPE_GZIP) {
			gzflush(cl_gz, Z_SYNC_FLUSH);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
}

int CFile::PImpl::write(const void *buf, size_t size)
{
	int tp = cl_type;
	int ret = -1;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fwrite(buf, size, 1, cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzwrite(cl_gz, buf, size);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
	return ret;
}

int CFile::PImpl::seek(long offset, int whence)
{
	int ret = -1;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = fseek(cl_plain, offset, whence);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gzseek(cl_gz, offset, whence);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
	return ret;
}

long CFile::PImpl::tell()
{
	int ret = -1;
	int tp = cl_type;

	if (tp != CLF_TYPE_INVALID) {
		if (tp == CLF_TYPE_PLAIN) {
			ret = ftell(cl_plain);
		}
#ifdef USE_ZLIB
		if (tp == CLF_TYPE_GZIP) {
			ret = gztell(cl_gz);
		}
#endif // USE_ZLIB
	} else {
		errno = EBADF;
	}
	return ret;
}


/**
**  Find a file with its correct extension ("" or ".gz")
**
**  @param file      The string with the file path. Upon success, the string
**                   is replaced by the full filename with the correct extension.
**  @param filesize  Size of the file buffer
**
**  @return true if the file has been found.
*/
static bool FindFileWithExtension(std::array<char, PATH_MAX> &file)
{
	const std::filesystem::path filepath = file.data();

	if (std::filesystem::exists(filepath)) {
		return true;
	}

#ifdef USE_ZLIB // gzip in global shared directory
	std::filesystem::path filepath_gz = filepath;
	filepath_gz += ".gz";

	if (std::filesystem::exists(filepath_gz)) {
		strcpy_s(file.data(), PATH_MAX, filepath_gz.string().c_str());
		return true;
	}
#endif

	return false;
}

/**
**  Generate a filename into library.
**
**  Try current directory, user home directory, global directory.
**  This supports .gz and .zip.
**
**  @param file        Filename to open.
**  @param buffer      Allocated buffer for generated filename.
*/
static void LibraryFileName(const char *file, std::array<char, PATH_MAX> &buffer)
{
	// Absolute path or in current directory.
	strcpy_s(buffer.data(), PATH_MAX, file);
	if (buffer.front() == '/') {
		return;
	}
	if (FindFileWithExtension(buffer)) {
		return;
	}

	const std::string root_path_str = database::get()->get_root_path().string();

	// Try in map directory
	if (!CurrentMapPath.empty()) {
		if (CurrentMapPath.front() == '.' || CurrentMapPath.front() == '/') {
			strcpy_s(buffer.data(), PATH_MAX, CurrentMapPath.c_str());
			char *s = strrchr(buffer.data(), '/');
			if (s) {
				s[1] = '\0';
			}
			strcat_s(buffer.data(), PATH_MAX, file);
		} else {
			strcpy_s(buffer.data(), PATH_MAX, root_path_str.c_str());
			if (buffer.front() != 0) {
				strcat_s(buffer.data(), PATH_MAX, "/");
			}
			strcat_s(buffer.data(), PATH_MAX, CurrentMapPath.c_str());
			char *s = strrchr(buffer.data(), '/');
			if (s) {
				s[1] = '\0';
			}
			strcat_s(buffer.data(), PATH_MAX, file);
		}
		if (FindFileWithExtension(buffer)) {
			return;
		}
	}

	// In user home directory
	if (!GameName.empty()) {
		snprintf(buffer.data(), PATH_MAX, "%s/%s/%s", parameters::get()->GetUserDirectory().c_str(), GameName.c_str(), file);
		if (FindFileWithExtension(buffer)) {
			return;
		}
	}

	snprintf(buffer.data(), PATH_MAX, "%s/%s", parameters::get()->GetUserDirectory().c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
		
	// In global shared directory
	#ifndef __MORPHOS__
	snprintf(buffer.data(), PATH_MAX, "%s/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif
	// Support for graphics in default graphics dir.
	// They could be anywhere now, but check if they haven't
	// got full paths.
	snprintf(buffer.data(), PATH_MAX, "graphics/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer.data(), PATH_MAX, "%s/graphics/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for sounds in default sounds dir.
	// They could be anywhere now, but check if they haven't
	// got full paths.
	snprintf(buffer.data(), PATH_MAX, "sounds/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer.data(), PATH_MAX, "%s/sounds/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for music in the default music dir.
	snprintf(buffer.data(), PATH_MAX, "music/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	snprintf(buffer.data(), PATH_MAX, "%s/music/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif

	// Support for scripts in default scripts dir.
	sprintf(buffer.data(), "scripts/%s", file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#ifndef __MORPHOS__	
	sprintf(buffer.data(), "%s/scripts/%s", root_path_str.c_str(), file);
	if (FindFileWithExtension(buffer)) {
		return;
	}
	#endif
	//Wyrmgus start
	if (DLCFileEquivalency.find(std::string(file)) != DLCFileEquivalency.end()) { //if the file hasn't been found and it has an equivalent file, try to get that instead
		LibraryFileName(DLCFileEquivalency[std::string(file)].c_str(), buffer);
		return;
	}
	//Wyrmgus end

	DebugPrint("File '%s' not found\n" _C_ file);
	strcpy_s(buffer.data(), PATH_MAX, file);
}

extern std::string LibraryFileName(const char *file)
{
	std::array<char, PATH_MAX> buffer{};
	LibraryFileName(file, buffer);
	return buffer.data();
}

bool CanAccessFile(const char *filename)
{
	if (filename && filename[0] != '\0') {
		std::array<char, PATH_MAX> name{};
		name[0] = '\0';
		LibraryFileName(filename, name);
		if (name[0] == '\0') {
			return false;
		}

		const std::filesystem::path filepath = name.data();
		if (std::filesystem::exists(filepath)) {
			return true;
		}
	}

	return false;
}

/**
**  Generate a list of files within a specified directory
**
**  @param dirname  Directory to read.
**  @param fl       Filelist pointer.
**
**  @return the number of entries added to FileList.
*/
std::vector<FileList> ReadDataDirectory(const std::filesystem::path &dir_path, const int sortmode)
{
	std::vector<FileList> file_list;

	if (!std::filesystem::exists(dir_path)) {
		return file_list;
	}

	std::filesystem::directory_iterator dir_iterator(dir_path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_directory() && !dir_entry.is_regular_file()) {
			continue;
		}

		FileList nfl;

		nfl.name = dir_entry.path().filename().string();
		if (!dir_entry.is_directory()) {
			nfl.type = 1;
		}
		nfl.mtime = std::filesystem::last_write_time(dir_entry.path());
		nfl.sortmode = sortmode;
		// sorted insertion
		file_list.insert(std::lower_bound(file_list.begin(), file_list.end(), nfl), nfl);
	}

	return file_list;
}

void FileWriter::printf(const char *format, ...)
{
	// FIXME: hardcoded size
	std::array<char, 1024> buf{};

	va_list ap;
	va_start(ap, format);
	buf[sizeof(buf) - 1] = '\0';
	vsnprintf(buf.data(), sizeof(buf) - 1, format, ap);
	va_end(ap);
	write(buf.data(), strlen(buf.data()));
}


class RawFileWriter final : public FileWriter
{
	FILE *file;

public:
	explicit RawFileWriter(const std::string &filename)
	{
		file = fopen(filename.c_str(), "wb");
		if (!file) {
			fprintf(stderr, "Can't open file '%s' for writing\n", filename.c_str());
			throw FileException();
		}
	}

	virtual ~RawFileWriter()
	{
		if (file) { fclose(file); }
	}

	virtual int write(const char *data, unsigned int size)
	{
		return fwrite(data, size, 1, file);
	}
};

class GzFileWriter final : public FileWriter
{
	gzFile file;

public:
	explicit GzFileWriter(const std::string &filename)
	{
		file = gzopen(filename.c_str(), "wb9");
		if (!file) {
			fprintf(stderr, "Can't open file '%s' for writing\n", filename.c_str());
			throw FileException();
		}
	}

	virtual ~GzFileWriter()
	{
		if (file) { gzclose(file); }
	}

	virtual int write(const char *data, unsigned int size)
	{
		return gzwrite(file, data, size);
	}
};

/**
**  Create FileWriter
*/
std::unique_ptr<FileWriter> CreateFileWriter(const std::string &filename)
{
	if (strcasestr(filename.c_str(), ".gz")) {
		return std::make_unique<GzFileWriter>(filename);
	} else {
		return std::make_unique<RawFileWriter>(filename);
	}
}

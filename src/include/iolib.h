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
//      (c) Copyright 2000-2022 by Andreas Arens and Andrettin
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

#pragma once

#ifdef __MORPHOS__
#undef tell
#endif

/**
**  Exception thrown by FileWriter objects.
**/
class FileException
{
};

/**
**  Abstract class representing files one can write to.
*/
class FileWriter
{
public:
	virtual ~FileWriter() {}

	void printf(const char *format, ...) PRINTF_VAARG_ATTRIBUTE(2, 3); // Don't forget to count this

	virtual int write(const char *data, unsigned int size) = 0;
};


/**
**  Create a file writer object that works for the given file name.
**
**  If the file name ends with '.gz', the file writer returned
**  will compress the data with zlib.
*/
std::unique_ptr<FileWriter> CreateFileWriter(const std::string &filename);

/**
**  FileList struct used by directory access routine
*/
class FileList
{
public:
	bool operator < (const FileList &rhs) const
	{
		if (type != rhs.type) {
			return type < rhs.type;
		}
		if (sortmode == 1) {
			return mtime > rhs.mtime;
		}
		return name < rhs.name;
	}

public:
	std::string name;  /// Name of the file
	int type = 0;      /// Type of the file
	int sortmode = 0;  /// Sort by name if 0 or sort by modified time if 1
	std::filesystem::file_time_type mtime;      /// Modified time
};


/**
**  Defines a library file
**
**  @todo  zip archive support
*/
class CFile
{
public:
	CFile();
	~CFile();

	int open(const char *name, long flags);
	int close();
	void flush();
	int read(void *buf, size_t len);
	int seek(long offset, int whence);
	long tell();

	int printf(const char *format, ...) PRINTF_VAARG_ATTRIBUTE(2, 3); // Don't forget to count this
private:
	CFile(const CFile &rhs); // No implementation
	const CFile &operator = (const CFile &rhs); // No implementation
private:
	class PImpl;
	std::unique_ptr<PImpl> pimpl;
};

enum {
	CLF_TYPE_INVALID,  /// invalid file handle
	CLF_TYPE_PLAIN,    /// plain text file handle
	CLF_TYPE_GZIP
};

#define CL_OPEN_READ 0x1
#define CL_OPEN_WRITE 0x2
#define CL_WRITE_GZ 0x4

/// Build library path name
extern std::string LibraryFileName(const char *file);

extern bool CanAccessFile(const char *filename);

/// Read the contents of a directory
extern std::vector<FileList> ReadDataDirectory(const std::filesystem::path &dir_path, int sortmode);

/**
\file
\brief Windows-specific things.
\copyright
    Copyright 2017-2017, SINTEF Ocean and the Coral contributors.
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CORAL_FMI_WINDOWS_HPP
#define CORAL_FMI_WINDOWS_HPP

#ifdef _WIN32

#include <string>
#include <boost/filesystem.hpp>


namespace coral
{
namespace fmi
{


/**
 *  \brief
 *  Temporarily adds a path to the PATH environment variable for the current
 *  process.
 *
 *  The path is added to PATH when the class is instantiated, and removed
 *  again when the instance is destroyed.
 *
 *  The purpose of this class is to add an FMU's binaries/<platform> directory
 *  to Windows' DLL search path.  This solves a problem where Windows was
 *  unable t to locate some DLLs that are indirectly loaded.  Specifically,
 *  the problem has been observed when the main FMU model DLL runs Java code
 *  (through JNI), and that Java code loaded a second DLL, which again was
 *  linked to further DLLs.  The latter were located in the binaries/<platform>
 *  directory, but were not found by the dynamic loader because that directory
 *  was not in the search path.
 *
 *  Since environment variables are shared by the entire process, the functions
 *  use a mutex to protect against concurrent access to the PATH variable while
 *  it's being read, modified and written.  (This does not protect against
 *  access by client code, of course, which is a potential source of bugs.
 *  See VIPROMA-67 for more info.)
 */
class AdditionalPath
{
public:
    /// Constructor. Adds `p` to `PATH`.
    AdditionalPath(const boost::filesystem::path& p);

    /// Destructor.  Removes the path from `PATH` again.
    ~AdditionalPath();

private:
    std::wstring m_addedPath;
};


/// Given `path/to/fmu`, returns `path/to/fmu/binaries/<platform>`
boost::filesystem::path FMUBinariesDir(const boost::filesystem::path& baseDir);


}} // namespace
#endif // _WIN32
#endif // header guard

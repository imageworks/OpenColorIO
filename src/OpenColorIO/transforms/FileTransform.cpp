// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Logging.h"
#include "Mutex.h"
#include "ops/noop/NoOps.h"
#include "PathUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
FileTransformRcPtr FileTransform::Create()
{
    return FileTransformRcPtr(new FileTransform(), &deleter);
}

void FileTransform::deleter(FileTransform* t)
{
    delete t;
}

class FileTransform::Impl
{
public:
    TransformDirection m_dir{ TRANSFORM_DIR_FORWARD };
    Interpolation m_interp{ INTERP_UNKNOWN };
    std::string m_src;

    std::string m_cccid;
    CDLStyle m_cdlStyle{ CDL_TRANSFORM_DEFAULT };

    Impl()
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;
            m_interp = rhs.m_interp;
            m_src = rhs.m_src;
            m_cccid = rhs.m_cccid;
            m_cdlStyle = rhs.m_cdlStyle;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

FileTransform::FileTransform()
    : m_impl(new FileTransform::Impl)
{
}

TransformRcPtr FileTransform::createEditableCopy() const
{
    FileTransformRcPtr transform = FileTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

FileTransform::~FileTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection FileTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void FileTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void FileTransform::validate() const
{
    Transform::validate();

    if (getImpl()->m_src.empty())
    {
        throw Exception("FileTransform: empty file path");
    }
}

const char * FileTransform::getSrc() const
{
    return getImpl()->m_src.c_str();
}

void FileTransform::setSrc(const char * src)
{
    getImpl()->m_src = src;
}

const char * FileTransform::getCCCId() const
{
    return getImpl()->m_cccid.c_str();
}

void FileTransform::setCCCId(const char * cccid)
{
    getImpl()->m_cccid = cccid;
}

CDLStyle FileTransform::getCDLStyle() const
{
    return getImpl()->m_cdlStyle;
}

void FileTransform::setCDLStyle(CDLStyle style)
{
    getImpl()->m_cdlStyle = style;
}

Interpolation FileTransform::getInterpolation() const
{
    return getImpl()->m_interp;
}

void FileTransform::setInterpolation(Interpolation interp)
{
    getImpl()->m_interp = interp;
}

int FileTransform::getNumFormats()
{
    return FormatRegistry::GetInstance().getNumFormats(
        FORMAT_CAPABILITY_READ);
}

const char * FileTransform::getFormatNameByIndex(int index)
{
    return FormatRegistry::GetInstance().getFormatNameByIndex(
        FORMAT_CAPABILITY_READ, index);
}

const char * FileTransform::getFormatExtensionByIndex(int index)
{
    return FormatRegistry::GetInstance().getFormatExtensionByIndex(
        FORMAT_CAPABILITY_READ, index);
}

std::ostream& operator<< (std::ostream& os, const FileTransform& t)
{
    os << "<FileTransform ";
    os << "direction=";
    os << TransformDirectionToString(t.getDirection()) << ", ";
    os << "interpolation=" << InterpolationToString(t.getInterpolation());
    os << ", src=" << t.getSrc() << ", ";
    os << "cccid=" << t.getCCCId();
    os << "cdl_style=" << CDLStyleToString(t.getCDLStyle());
    os << ">";

    return os;
}

///////////////////////////////////////////////////////////////////////////

// NOTE: You must be mindful when editing this function.
//       to be resiliant to the static initialization order 'fiasco'
//
//       See
//       http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.14
//       http://stackoverflow.com/questions/335369/finding-c-static-initialization-order-problems
//       for more info.

namespace
{
FormatRegistry* g_formatRegistry = NULL;
Mutex g_formatRegistryLock;
}

FormatRegistry & FormatRegistry::GetInstance()
{
    AutoMutex lock(g_formatRegistryLock);

    if(!g_formatRegistry)
    {
        g_formatRegistry = new FormatRegistry();
    }

    return *g_formatRegistry;
}

FormatRegistry::FormatRegistry()
{
    registerFileFormat(CreateFileFormat3DL());
    registerFileFormat(CreateFileFormatCC());
    registerFileFormat(CreateFileFormatCCC());
    registerFileFormat(CreateFileFormatCDL());
    registerFileFormat(CreateFileFormatCLF());
    registerFileFormat(CreateFileFormatCSP());
    registerFileFormat(CreateFileFormatDiscreet1DL());
    registerFileFormat(CreateFileFormatHDL());
    registerFileFormat(CreateFileFormatICC());
    registerFileFormat(CreateFileFormatIridasCube());
    registerFileFormat(CreateFileFormatIridasItx());
    registerFileFormat(CreateFileFormatIridasLook());
    registerFileFormat(CreateFileFormatPandora());
    registerFileFormat(CreateFileFormatResolveCube());
    registerFileFormat(CreateFileFormatSpi1D());
    registerFileFormat(CreateFileFormatSpi3D());
    registerFileFormat(CreateFileFormatSpiMtx());
    registerFileFormat(CreateFileFormatTruelight());
    registerFileFormat(CreateFileFormatVF());
}

FormatRegistry::~FormatRegistry()
{
}

FileFormat* FormatRegistry::getFileFormatByName(const std::string & name) const
{
    FileFormatMap::const_iterator iter = m_formatsByName.find(StringUtils::Lower(name));
    if(iter != m_formatsByName.end())
        return iter->second;

    return nullptr;
}

void FormatRegistry::getFileFormatForExtension(const std::string & extension,
                                               FileFormatVector & possibleFormats) const
{
    FileFormatVectorMap::const_iterator iter
        = m_formatsByExtension.find(StringUtils::Lower(extension));

    if(iter != m_formatsByExtension.end())
        possibleFormats = iter->second;
}

void FormatRegistry::registerFileFormat(FileFormat* format)
{
    FormatInfoVec formatInfoVec;
    format->getFormatInfo(formatInfoVec);

    if(formatInfoVec.empty())
    {
        std::ostringstream os;
        os << "FileFormat Registry error. ";
        os << "A file format did not provide the required format info.";
        throw Exception(os.str().c_str());
    }

    for(unsigned int i=0; i<formatInfoVec.size(); ++i)
    {
        if(formatInfoVec[i].capabilities == FORMAT_CAPABILITY_NONE)
        {
            std::ostringstream os;
            os << "FileFormat Registry error. ";
            os << "A file format does not define either";
            os << " reading or writing.";
            throw Exception(os.str().c_str());
        }

        if(getFileFormatByName(formatInfoVec[i].name))
        {
            std::ostringstream os;
            os << "Cannot register multiple file formats named, '";
            os << formatInfoVec[i].name << "'.";
            throw Exception(os.str().c_str());
        }

        m_formatsByName[StringUtils::Lower(formatInfoVec[i].name)] = format;

        m_formatsByExtension[formatInfoVec[i].extension].push_back(format);

        if(formatInfoVec[i].capabilities & FORMAT_CAPABILITY_READ)
        {
            m_readFormatNames.push_back(formatInfoVec[i].name);
            m_readFormatExtensions.push_back(formatInfoVec[i].extension);
        }

        if (formatInfoVec[i].capabilities & FORMAT_CAPABILITY_BAKE)
        {
            m_bakeFormatNames.push_back(formatInfoVec[i].name);
            m_bakeFormatExtensions.push_back(formatInfoVec[i].extension);
        }

        if(formatInfoVec[i].capabilities & FORMAT_CAPABILITY_WRITE)
        {
            m_writeFormatNames.push_back(formatInfoVec[i].name);
            m_writeFormatExtensions.push_back(formatInfoVec[i].extension);
        }
    }

    m_rawFormats.push_back(format);
}

int FormatRegistry::getNumRawFormats() const
{
    return static_cast<int>(m_rawFormats.size());
}

FileFormat* FormatRegistry::getRawFormatByIndex(int index) const
{
    if(index<0 || index>=getNumRawFormats())
    {
        return NULL;
    }

    return m_rawFormats[index];
}

int FormatRegistry::getNumFormats(int capability) const
{
    if(capability == FORMAT_CAPABILITY_READ)
    {
        return static_cast<int>(m_readFormatNames.size());
    }
    else if (capability == FORMAT_CAPABILITY_BAKE)
    {
        return static_cast<int>(m_bakeFormatNames.size());
    }
    else if (capability == FORMAT_CAPABILITY_WRITE)
    {
        return static_cast<int>(m_writeFormatNames.size());
    }
    return 0;
}

const char * FormatRegistry::getFormatNameByIndex(
    int capability, int index) const
{
    if(capability == FORMAT_CAPABILITY_READ)
    {
        if(index<0 || index>=static_cast<int>(m_readFormatNames.size()))
        {
            return "";
        }
        return m_readFormatNames[index].c_str();
    }
    else if (capability == FORMAT_CAPABILITY_BAKE)
    {
        if (index<0 || index >= static_cast<int>(m_bakeFormatNames.size()))
        {
            return "";
        }
        return m_bakeFormatNames[index].c_str();
    }
    else if(capability == FORMAT_CAPABILITY_WRITE)
    {
        if(index<0 || index>=static_cast<int>(m_writeFormatNames.size()))
        {
            return "";
        }
        return m_writeFormatNames[index].c_str();
    }
    return "";
}

const char * FormatRegistry::getFormatExtensionByIndex(
    int capability, int index) const
{
    if(capability == FORMAT_CAPABILITY_READ)
    {
        if(index<0 
            || index>=static_cast<int>(m_readFormatExtensions.size()))
        {
            return "";
        }
        return m_readFormatExtensions[index].c_str();
    }
    else if (capability == FORMAT_CAPABILITY_BAKE)
    {
        if (index<0
            || index >= static_cast<int>(m_bakeFormatExtensions.size()))
        {
            return "";
        }
        return m_bakeFormatExtensions[index].c_str();
    }
    else if(capability == FORMAT_CAPABILITY_WRITE)
    {
        if(index<0 
            || index>=static_cast<int>(m_writeFormatExtensions.size()))
        {
            return "";
        }
        return m_writeFormatExtensions[index].c_str();
    }
    return "";
}

///////////////////////////////////////////////////////////////////////////

FileFormat::~FileFormat()
{

}

std::string FileFormat::getName() const
{
    FormatInfoVec infoVec;
    getFormatInfo(infoVec);
    if(infoVec.size()>0)
    {
        return infoVec[0].name;
    }
    return "Unknown Format";
}

void FileFormat::bake(const Baker & /*baker*/,
                      const std::string & formatName,
                      std::ostream & /*ostream*/) const
{
    std::ostringstream os;
    os << "Format '" << formatName << "' does not support baking.";
    throw Exception(os.str().c_str());
}

void FileFormat::write(const OpRcPtrVec & /*ops*/,
                       const FormatMetadataImpl & /*metadata*/,
                       const std::string & formatName,
                       std::ostream & /*ostream*/) const
{
    std::ostringstream os;
    os << "Format '" << formatName << "' does not support writing.";
    throw Exception(os.str().c_str());
}

namespace
{

void LoadFileUncached(FileFormat * & returnFormat,
                      CachedFileRcPtr & returnCachedFile,
                      const std::string & filepath)
{
    returnFormat = NULL;

    {
        std::ostringstream oss;
        oss << "**" << std::endl
            << "Opening " << filepath;
        LogDebug(oss.str());
    }

    // Try the initial format.
    std::string primaryErrorText("\n"); // Add a separator for the first reader error.

    std::string root, extension;
    pystring::os::path::splitext(root, extension, filepath);
    // remove the leading '.'
    extension = pystring::replace(extension,".","",1);

    FormatRegistry & formatRegistry = FormatRegistry::GetInstance();

    FileFormatVector possibleFormats;
    formatRegistry.getFileFormatForExtension(extension, possibleFormats);
    FileFormatVector::const_iterator endFormat = possibleFormats.end();
    FileFormatVector::const_iterator itFormat = possibleFormats.begin();
    while(itFormat != endFormat)
    {

        FileFormat * tryFormat = *itFormat;
        std::ifstream filestream;
        try
        {
            // Open the filePath
            filestream.open(
                filepath.c_str(),
                tryFormat->isBinary()
                    ? std::ios_base::binary : std::ios_base::in);
            if (!filestream.good())
            {
                std::ostringstream os;
                os << "The specified FileTransform srcfile, '";
                os << filepath << "', could not be opened. ";
                os << "Please confirm the file exists with ";
                os << "appropriate read permissions.";
                throw Exception(os.str().c_str());
            }

            CachedFileRcPtr cachedFile = tryFormat->read(
                filestream,
                filepath);

            if(IsDebugLoggingEnabled())
            {
                std::ostringstream os;
                os << "    Loaded primary format ";
                os << tryFormat->getName() << std::endl;
                LogDebug(os.str());
            }

            returnFormat = tryFormat;
            returnCachedFile = cachedFile;
            filestream.close();
            return;
        }
        catch(std::exception & e)
        {
            if (filestream.is_open())
            {
                filestream.close();
            }

            primaryErrorText += "\t'";
            primaryErrorText += tryFormat->getName();
            primaryErrorText += "' failed with: ";
            primaryErrorText += e.what();

            if(IsDebugLoggingEnabled())
            {
                std::ostringstream os;
                os << "    Failed primary format ";
                os << tryFormat->getName();
                os << ":  " << e.what();
                LogDebug(os.str());
            }
        }
        ++itFormat;
    }

    // If this fails, try all other formats
    CachedFileRcPtr cachedFile;
    FileFormat * altFormat = NULL;

    for(int findex = 0;
        findex<formatRegistry.getNumRawFormats();
        ++findex)
    {
        altFormat = formatRegistry.getRawFormatByIndex(findex);

        // Do not try primary formats twice.
        FileFormatVector::const_iterator itAlt = std::find(
            possibleFormats.begin(), possibleFormats.end(), altFormat);
        if(itAlt != endFormat)
            continue;

        std::ifstream filestream;
        try
        {
            filestream.open(filepath.c_str(), altFormat->isBinary()
                ? std::ios_base::binary : std::ios_base::in);
            if (!filestream.good())
            {
                std::ostringstream os;
                os << "The specified FileTransform srcfile, '";
                os << filepath << "', could not be opened. ";
                os << "Please confirm the file exists with ";
                os << "appropriate read";
                os << " permissions.";
                throw Exception(os.str().c_str());
            }

            cachedFile = altFormat->read(filestream, filepath);

            if(IsDebugLoggingEnabled())
            {
                std::ostringstream os;
                os << "    Loaded alt format ";
                os << altFormat->getName();
                LogDebug(os.str());
            }

            returnFormat = altFormat;
            returnCachedFile = cachedFile;
            filestream.close();
            return;
        }
        catch(std::exception & e)
        {
            if (filestream.is_open())
            {
                filestream.close();
            }

            if(IsDebugLoggingEnabled())
            {
                std::ostringstream os;
                os << "    Failed alt format ";
                os << altFormat->getName();
                os << ":  " << e.what();
                LogDebug(os.str());
            }
        }
    }

    // No formats succeeded. Error out with a sensible message.
    std::ostringstream os;
    os << "The specified transform file '";
    os << filepath << "' could not be loaded.\n";
    os << "All formats have been tried. ";

    if (IsDebugLoggingEnabled())
    {
        os << "(Refer to debug log for errors from all formats.) ";
    }
    else
    {
        os << "(Enable debug log for errors from all formats.) ";
    }

    if(!possibleFormats.empty())
    {
        if (possibleFormats.size() == 1)
        {
            os << "The format for the file's extension gave the error:\n";
        }
        else
        {
            os << "The formats for the file's extension gave the errors:\n";
        }
        os << primaryErrorText;
    }

    throw Exception(os.str().c_str());
}

// We mutex both the main map and each item individually, so that
// the potentially slow file access wont block other lookups to already
// existing items. (Loads of the *same* file will mutually block though)

struct FileCacheResult
{
    Mutex mutex;
    FileFormat * format;
    bool ready;
    bool error;
    CachedFileRcPtr cachedFile;
    std::string exceptionText;

    FileCacheResult():
        format(NULL),
        ready(false),
        error(false)
    {}
};

typedef OCIO_SHARED_PTR<FileCacheResult> FileCacheResultPtr;
typedef std::map<std::string, FileCacheResultPtr> FileCacheMap;

FileCacheMap g_fileCache;
Mutex g_fileCacheLock;

} // namespace

void GetCachedFileAndFormat(FileFormat * & format,
                            CachedFileRcPtr & cachedFile,
                            const std::string & filepath)
{
    // Load the file cache ptr from the global map
    FileCacheResultPtr result;
    {
        AutoMutex lock(g_fileCacheLock);
        FileCacheMap::iterator iter = g_fileCache.find(filepath);
        if (iter != g_fileCache.end())
        {
            result = iter->second;
        }
        else
        {
            result = FileCacheResultPtr(new FileCacheResult);
            g_fileCache[filepath] = result;
        }
    }

    // If this file has already been loaded, return
    // the result immediately

    AutoMutex lock(result->mutex);
    if (!result->ready)
    {
        result->ready = true;
        result->error = false;

        try
        {
            LoadFileUncached(result->format,
                result->cachedFile,
                filepath);
        }
        catch (std::exception & e)
        {
            result->error = true;
            result->exceptionText = e.what();
        }
        catch (...)
        {
            result->error = true;
            std::ostringstream os;
            os << "An unknown error occurred in LoadFileUncached, ";
            os << filepath;
            result->exceptionText = os.str();
        }
    }

    if (result->error)
    {
        throw Exception(result->exceptionText.c_str());
    }
    else
    {
        format = result->format;
        cachedFile = result->cachedFile;
    }

    if (!format)
    {
        std::ostringstream os;
        os << "The specified file load ";
        os << filepath << " appeared to succeed, but no format ";
        os << "was returned.";
        throw Exception(os.str().c_str());
    }

    if (!cachedFile.get())
    {
        std::ostringstream os;
        os << "The specified file load ";
        os << filepath << " appeared to succeed, but no cachedFile ";
        os << "was returned.";
        throw Exception(os.str().c_str());
    }
}

void ClearFileTransformCaches()
{
    AutoMutex lock(g_fileCacheLock);
    g_fileCache.clear();
}

void BuildFileTransformOps(OpRcPtrVec & ops,
                           const Config& config,
                           const ConstContextRcPtr & context,
                           const FileTransform& fileTransform,
                           TransformDirection dir)
{
    std::string src = fileTransform.getSrc();
    if(src.empty())
    {
        std::ostringstream os;
        os << "The transform file has not been specified.";
        throw Exception(os.str().c_str());
    }

    std::string filepath = context->resolveFileLocation(src.c_str());

    // Verify the recursion is valid, FileNoOp is added for each file.
    for (const OpRcPtr & op : ops)
    {
        ConstOpRcPtr const_op(op);
        ConstOpDataRcPtr data = const_op->data();
        auto fileData = DynamicPtrCast<const FileNoOpData>(data);
        if (fileData)
        {
            // Error if file is still being loaded and is the same as the
            // one about to be loaded.
            if (!fileData->getComplete() &&
                Platform::Strcasecmp(fileData->getPath().c_str(), filepath.c_str()) == 0)
            {
                std::ostringstream os;
                os << "Reference to: " << filepath;
                os << " is creating a recursion.";

                throw Exception(os.str().c_str());
            }
        }
    }

    FileFormat* format = NULL;
    CachedFileRcPtr cachedFile;

    GetCachedFileAndFormat(format, cachedFile, filepath);

    try
    {
        // Add FileNoOp and keep track of it.
        CreateFileNoOp(ops, filepath);
        ConstOpRcPtr fileNoOpConst = ops.back();
        OpRcPtr fileNoOp = ops.back();

        // CTF implementation of FileFormat::buildFileOps might call
        // BuildFileTransformOps for References.
        format->buildFileOps(ops,
                                config, context,
                                cachedFile, fileTransform,
                                dir);

        // File has been loaded completely. It may now be referenced again.
        ConstOpDataRcPtr data = fileNoOpConst->data();
        auto fileData = DynamicPtrCast<const FileNoOpData>(data);
        if (fileData)
        {
            fileData->setComplete();
        }
    }
    catch (Exception & e)
    {
        std::ostringstream err;
        err << "The transform file: " << filepath;
        err << " failed while building ops with this error: ";
        err << e.what();
        throw Exception(err.str().c_str());
    }
}
} // namespace OCIO_NAMESPACE

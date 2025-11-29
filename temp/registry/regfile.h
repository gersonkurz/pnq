#pragma once

#include <ngbtools/win32/registry/regfile_importer.h>

namespace ngbtools
{
    namespace win32
    {
        namespace registry
        {
            /**
             * \brief   Given a filename, identify the proper registry importer
             *
             * \param   filename        Filename of the file.
             * \param   import_options  Registry import options.
             *
             * \return  Registry importer suitable for this file
             */

            regfile_import_interface* create_importer_from_file(const std::string& filename, REGFILE_IMPORT_OPTIONS import_options);
            regfile_import_interface* create_importer_from_string(const std::string& content, REGFILE_IMPORT_OPTIONS import_options);
        }
    }
}

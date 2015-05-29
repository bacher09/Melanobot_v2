/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "python.hpp"

#include "python-utils.hpp"

namespace python {


ScriptOutput PythonEngine::exec(const std::string& python_code)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;

    try {
        ScriptEnvironment env(output);
        boost::python::exec({python_code.data(), python_code.length()}, env.main_namespace());
        output.success = true;
    } catch (const boost::python::error_already_set&) {
        ErrorLog("py") << "Exception from python script";
        PyErr_Print();
    }

    return output;
}

ScriptOutput PythonEngine::exec_file(const std::string& file)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;

    try {
        ScriptEnvironment env(output);
        boost::python::exec_file({file.data(), file.length()}, env.main_namespace());
        output.success = true;
    } catch (const boost::python::error_already_set&) {
        ErrorLog("py") << "Exception from python script";
        PyErr_Print();
    }

    return output;
}

bool PythonEngine::initialize()
{
    if ( !Py_IsInitialized() )
        Py_Initialize();

    if ( !Py_IsInitialized() )
    {
        ErrorLog("py") << "Could not initialize the interpreter";
        return false;
    }

    return true;
}

} // namespace python

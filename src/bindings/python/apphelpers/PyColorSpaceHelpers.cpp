// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
namespace
{

enum MenuIterator
{
    IT_MENU_HIERARCHY_LEVEL = 0
};

using ColorSpaceLevelIterator = PyIterator<ColorSpaceMenuHelperRcPtr,
                                           IT_MENU_HIERARCHY_LEVEL, size_t>;

} // namespace

void bindPyColorSpaceMenuHelpers(py::module & m)
{
    m.def_submodule("ColorSpaceHelpers")
        .def("AddColorSpace", &ColorSpaceHelpers::AddColorSpace,
             "config"_a.none(false),
             "name"_a.none(false),
             "transformFilePath"_a.none(false),
             "categories"_a = "",
             "connectionColorSpaceName"_a.none(false));
    auto nsCat = m.def_submodule("Category");
    nsCat.attr("INPUT") = Category::INPUT;
    nsCat.attr("SCENE_LINEAR_WORKING_SPACE") = Category::SCENE_LINEAR_WORKING_SPACE;
    nsCat.attr("LOG_WORKING_SPACE") = Category::LOG_WORKING_SPACE;
    nsCat.attr("VIDEO_WORKING_SPACE") = Category::VIDEO_WORKING_SPACE;
    nsCat.attr("LUT_INPUT_SPACE") = Category::LUT_INPUT_SPACE;

    auto cls = py::class_<ColorSpaceMenuHelper,
        ColorSpaceMenuHelperRcPtr /* holder */>(m, "ColorSpaceMenuHelper");

    py::enum_<ColorSpaceMenuHelper::IncludeTypeFlag>(cls, "IncludeTypeFlag", py::arithmetic())
        .value("INCLUDE_NO_EXTRAS", ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS)
        .value("INCLUDE_ROLES", ColorSpaceMenuHelper::INCLUDE_ROLES)
        .value("INCLUDE_NAMEDTRANSFORMS", ColorSpaceMenuHelper::INCLUDE_NAMEDTRANSFORMS)
        .value("INCLUDE_ALL_EXTRAS", ColorSpaceMenuHelper::INCLUDE_ALL_EXTRAS)
        .export_values();

    cls.def(py::init(&ColorSpaceMenuHelper::Create),
                     "config"_a.none(false), "role"_a = "", "categories"_a = "",
                     "includeFlag"_a = ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS)
        .def("getNumColorSpaces", &ColorSpaceMenuHelper::getNumColorSpaces)
        .def("getName", &ColorSpaceMenuHelper::getName, "index"_a)
        .def("getUIName", &ColorSpaceMenuHelper::getUIName, "index"_a)
        .def("getIndexFromName", &ColorSpaceMenuHelper::getIndexFromName, "name"_a.none(false))
        .def("getIndexFromUIName", &ColorSpaceMenuHelper::getIndexFromUIName, "name"_a.none(false))
        .def("getDescription", &ColorSpaceMenuHelper::getDescription, "index"_a)
        .def("getFamily", &ColorSpaceMenuHelper::getFamily, "index"_a)
        .def("getHierarchyLevels",
             [](ColorSpaceMenuHelperRcPtr & self, size_t index)
             {
                 return ColorSpaceLevelIterator(self, index);
             }, "index"_a)
        .def("getNameFromUIName", &ColorSpaceMenuHelper::getNameFromUIName, "name"_a.none(false))
        .def("getUINameFromName", &ColorSpaceMenuHelper::getUINameFromName, "name"_a.none(false))
        .def("addColorSpaceToMenu", &ColorSpaceMenuHelper::addColorSpaceToMenu, "name"_a.none(false))
        .def("refresh", &ColorSpaceMenuHelper::refresh, "config"_a.none(false));

    defStr(cls);

    py::class_<ColorSpaceLevelIterator>(cls, "ColorSpaceLevelIterator")
        .def("__len__", [](ColorSpaceLevelIterator & it)
             {
                 return it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args));
             })
        .def("__getitem__", [](ColorSpaceLevelIterator & it, int i)
             {
                 it.checkIndex(i, static_cast<int>(it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args))));
                 return it.m_obj->getHierarchyLevel(std::get<0>(it.m_args), static_cast<size_t>(i));
             })
        .def("__iter__", [](ColorSpaceLevelIterator & it) -> ColorSpaceLevelIterator &
             {
                 return it;
             })
        .def("__next__", [](ColorSpaceLevelIterator & it)
             {
                 int i = it.nextIndex(static_cast<int>(it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args))));
                 return it.m_obj->getHierarchyLevel(std::get<0>(it.m_args), static_cast<size_t>(i));
             });

}
}

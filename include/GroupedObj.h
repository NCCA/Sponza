/*
  Copyright (C) 2009 Jon Macey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GROUPEDOBJ_H_
#define GROUPEDOBJ_H_
//----------------------------------------------------------------------------------------------------------------------
/// @file GroupedObj.h
/// @brief This class loads in an Alias Obj file containing groups of meshes
/// it will extract each group and stored the name and associated material for each one
/// but still store the data as a single VertexArrayObjet. This allows for quick transfer of
/// data to and from the GPU but switching of model materials / textures. The mesh offset data is stored
/// in order of material name to reduce switching of textures / materials as well.
/// we also have iterator access to the internal data which will give name / material info as well
/// as the offset to the data in the VAO. Meshes with Quads will be split to triangles. Other meshes will give
/// errors but not crash.
/// @author Jonathan Macey
/// @version 1.0
/// @date 5/11/12
/// @todo write code to serialise the data in a binary format, best to store the VAO as a single blob and then
/// re-create it later.
/// Revision History :

//----------------------------------------------------------------------------------------------------------------------
// must include types.h first for ngl::Real and GLEW if required
#include <ngl/Types.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <ngl/Obj.h>
#include "VAO.h"
#include <cmath>

/// @brief simple data structure to store the Mesh information, it has an overloaded < operator
/// to allow for sorting by the Material name type
struct MeshData
{
  /// @brief the name of the mesh group from the obj file
  std::string m_name;
  /// @brief the name of the material to use
  std::string m_material;
  /// @brief the starting index of the group in the VertexArrayObject
  size_t m_startIndex;
  /// @brief the number of vertices to draw from the start index
  size_t m_numVerts;
  /// @brief overloaded < operator for mesh sorting
  bool operator<(const MeshData &_r) const { return m_material < _r.m_material; }
};

class GroupedObj : public ngl::Obj
{
public:
  GroupedObj(const std::string &_fname);
  bool load(const std::string &_fname, CalcBB _calcBB = CalcBB::True) noexcept override;
  void debugPrint();
  void draw(size_t _meshID) const;
  size_t numMeshes() const;
  std::string getMaterial(unsigned int _m) const;
  std::string getName(unsigned int _m) const;
  bool parseGroup(std::vector<std::string> &_tokens) noexcept;
  bool parseMaterial(std::vector<std::string> &_tokens) noexcept;
  bool parseFace(std::vector<std::string> &_tokens) noexcept override;

private:
  std::vector<MeshData> m_meshes;
  MeshData m_currentMesh;
  std::string m_currentMeshName;
  std::string m_currentMaterial;
  unsigned int m_faceCount;
  unsigned int m_offset;
  void createVAO(ResetVAO _reset = ResetVAO::False) noexcept override;
};

#endif

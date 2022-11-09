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
#include "GroupedObj.h"
#include <ngl/NGLMessage.h>
#include <ngl/VAOFactory.h>
#include <ngl/pystring.h>
namespace ps = pystring;

// a simple structure to hold our vertex data
struct VertData
{
  GLfloat x; // position from obj
  GLfloat y;
  GLfloat z;
  GLfloat nx; // normal from obj mesh
  GLfloat ny;
  GLfloat nz;
  GLfloat u; // tex cords
  GLfloat v; // tex cords
};

GroupedObj::GroupedObj(const std::string &_fname)
{
  m_loaded = load(_fname, CalcBB::True);
  // as the face triggers the push back of the meshes once we have finished the load we need to add the rest
  m_currentMesh.m_material = m_currentMaterial;
  m_currentMesh.m_name = m_currentMeshName;
  m_currentMesh.m_numVerts = m_faceCount;
  // index into the VAO data 3 tris with uv, normal and x,y,z as floats
  m_currentMesh.m_startIndex = m_offset;
  m_meshes.push_back(m_currentMesh);
  std::sort(m_meshes.begin(), m_meshes.end());
  createVAO();
}
bool GroupedObj::load(const std::string &_fname, CalcBB _calcBB) noexcept
{
  m_faceCount = 0;
  m_currentMesh.m_startIndex = 0;
  m_currentMesh.m_numVerts = 0;
  std::ifstream in(_fname.data());
  if (in.is_open() != true)
  {
    ngl::NGLMessage::addError(fmt::format(" file {0} not found  ", _fname.data()));
    return false;
  }

  std::string str;
  // Read the next line from File untill it reaches the end.
  // while (std::getline(in, str))
  while (!safeGetline(in, str).eof())
  {
    bool status = true;
    // Line contains string of length > 0 then parse it
    if (str.size() > 0)
    {
      std::vector<std::string> tokens;
      ps::split(str, tokens);
      if (tokens[0] == "v")
      {
        status = parseVertex(tokens);
      }
      else if (tokens[0] == "vn")
      {
        status = parseNormal(tokens);
      }

      else if (tokens[0] == "vt")
      {
        status = parseUV(tokens);
      }
      else if (tokens[0] == "f")
      {
        status = parseFace(tokens);
      }
      else if (tokens[0] == "g")
      {
        status = parseGroup(tokens);
      }
      else if (tokens[0] == "usemtl")
      {
        status = parseMaterial(tokens);
      }

    } // str.size()
    // early out sanity checks!
    if (status == false)
      return false;
  } // while

  in.close();
  // Calculate the center of the object.
  if (_calcBB == CalcBB::True)
  {
    this->calcDimensions();
  }
  m_isLoaded = true;
  return true;
}

bool GroupedObj::parseGroup(std::vector<std::string> &_tokens) noexcept
{

  // as the group is defined then the face data follows
  // we need to load the first set of faces before saving the data
  // this static int does that
  static int firstTime = 0;

  //  std::cout << "current group " << m_currentMeshName << "\n";
  if (firstTime++ > 0)
  {
    // now we add the group to our list
    m_currentMesh.m_material = m_currentMaterial;
    m_currentMesh.m_name = m_currentMeshName;
    m_currentMesh.m_numVerts = m_faceCount;
    // index into the VAO data 3 tris with uv, normal and x,y,z as floats
    m_currentMesh.m_startIndex = m_offset;
    // std::cout << "Stored values fc " << m_faceCount << "\n";
    // std::cout << "offset " << m_offset << "\n";
    m_meshes.push_back(m_currentMesh);
    m_offset += m_faceCount;
    m_faceCount = 0;
  }

  // set current group
  if (_tokens.size() == 1)
  {
    m_currentMeshName = "none";
  }
  else
  {
    m_currentMeshName = _tokens[1];
  }

  return true;
}
bool GroupedObj::parseMaterial(std::vector<std::string> &_tokens) noexcept
{
  if (_tokens.size() == 1)
  {
    m_currentMaterial = "default";
  }
  else
  {
    m_currentMaterial = ps::strip(_tokens[1], " \t\n\r");
  }

  return true;
}

void GroupedObj::debugPrint()
{
  for (auto m : m_meshes)
  {
    std::cout << "------------------------------------\n";
    std::cout << "Name " << m.m_name << "\n";
    std::cout << "Material " << m.m_material << "\n";
    std::cout << "Num verts " << m.m_numVerts << "\n";
    std::cout << "Start Index " << m.m_startIndex << "\n";
    std::cout << "------------------------------------\n";
  }
}
void GroupedObj::draw(size_t _meshID) const
{
  m_vaoMesh->bind();

  reinterpret_cast<VAO *>(m_vaoMesh.get())->draw(m_meshes[_meshID].m_startIndex, m_meshes[_meshID].m_numVerts);

  m_vaoMesh->unbind();
}
size_t GroupedObj::numMeshes() const
{
  return m_meshes.size();
}
std::string GroupedObj::getMaterial(unsigned int _m) const
{
  return m_meshes[_m].m_material;
}
std::string GroupedObj::getName(unsigned int _m) const
{
  return m_meshes[_m].m_name;
}

void GroupedObj::createVAO(ResetVAO _reset) noexcept
{
  // else allocate space as build our VAO
  m_dataPackType = 0;
  if (isTriangular())
  {
    m_dataPackType = GL_TRIANGLES;
    std::cout << "Doing Tri Data" << std::endl;
  }
  // data is mixed of > quad so exit error
  if (m_dataPackType == 0)
  {
    std::cerr << "Can only create VBO from all Triangle or ALL Quad data at present" << std::endl;
    exit(EXIT_FAILURE);
  }

  // now we are going to process and pack the mesh into an ngl::VertexArrayObject
  std::vector<VertData> vboMesh;
  VertData d;
  size_t loopFaceCount = 3;

  // loop for each of the faces
  for (size_t i = 0; i < m_face.size(); ++i)
  {
    // now for each triangle in the face (remember we ensured tri above)
    for (size_t j = 0; j < loopFaceCount; ++j)
    {

      // pack in the vertex data first
      d.x = m_verts[m_face[i].m_vert[j]].m_x;
      d.y = m_verts[m_face[i].m_vert[j]].m_y;
      d.z = m_verts[m_face[i].m_vert[j]].m_z;
      // now if we have norms of tex (possibly could not) pack them as well
      if (m_norm.size() > 0 && m_uv.size() > 0)
      {

        d.nx = m_norm[m_face[i].m_norm[j]].m_x;
        d.ny = m_norm[m_face[i].m_norm[j]].m_y;
        d.nz = m_norm[m_face[i].m_norm[j]].m_z;

        d.u = m_uv[m_face[i].m_uv[j]].m_x;
        d.v = m_uv[m_face[i].m_uv[j]].m_y;
      }
      // now if neither are present (only verts like Zbrush models)
      else if (m_norm.size() == 0 && m_uv.size() == 0)
      {
        d.nx = 0;
        d.ny = 0;
        d.nz = 0;
        d.u = 0;
        d.v = 0;
      }
      // here we've got norms but not tex
      else if (m_norm.size() > 0 && m_uv.size() == 0)
      {
        d.nx = m_norm[m_face[i].m_norm[j]].m_x;
        d.ny = m_norm[m_face[i].m_norm[j]].m_y;
        d.nz = m_norm[m_face[i].m_norm[j]].m_z;
        d.u = 0;
        d.v = 0;
      }
      // here we've got tex but not norm least common
      else if (m_norm.size() == 0 && m_uv.size() > 0)
      {
        d.nx = 0;
        d.ny = 0;
        d.nz = 0;
        d.u = m_uv[m_face[i].m_uv[j]].m_x;
        d.v = m_uv[m_face[i].m_uv[j]].m_y;
      }
      vboMesh.push_back(d);
    }
  }

  // first we grab an instance of our VOA
  m_vaoMesh = ngl::VAOFactory::createVAO("sponzaVAO", m_dataPackType);
  // next we bind it so it's active for setting data
  m_vaoMesh->bind();
  m_meshSize = vboMesh.size();

  // now we have our data add it to the VAO, we need to tell the VAO the following
  // how much (in bytes) data we are copying
  // a pointer to the first element of data (in this case the address of the first element of the
  // std::vector
  m_vaoMesh->setData(VAO::VertexData(m_meshSize * sizeof(VertData), vboMesh[0].x));
  // in this case we have packed our data in interleaved format as follows
  // x,y,,z,nx,ny,nz,u,v
  m_vaoMesh->setVertexAttributePointer(0, 3, GL_FLOAT, sizeof(VertData), 0);
  // uv same as above but starts at 0 and is attrib 1 and only u,v so 2
  m_vaoMesh->setVertexAttributePointer(1, 3, GL_FLOAT, sizeof(VertData), 3);
  // normal same as vertex only starts at position 2 (u,v)-> nx
  m_vaoMesh->setVertexAttributePointer(2, 2, GL_FLOAT, sizeof(VertData), 6);

  // now we have set the vertex attributes we tell the VAO class how many indices to draw when
  // glDrawArrays is called, in this case we use buffSize (but if we wished less of the sphere to be drawn we could
  // specify less (in steps of 3))
  m_vaoMesh->setNumIndices(m_meshSize);
  // finally we have finished for now so time to unbind the VAO
  m_vaoMesh->unbind();

  // indicate we have a vao now
  m_vao = true;
}

bool GroupedObj::parseFace(std::vector<std::string> &_tokens) noexcept
{
  bool parsedOK = true;
  // first let's find what sort of face we are dealing with
  // I assume most likely case is all
  if (_tokens.size() == 4)
  {

    if (ps::count(_tokens[1], "/") == 2 && ps::find(_tokens[1], "//") == -1)
    {
      parsedOK = parseFaceVertexNormalUV(_tokens);
    }

    else if (ps::find(_tokens[1], "/") == -1)
    {
      parsedOK = parseFaceVertex(_tokens);
    }
    // look for VertNormal
    else if (ps::find(_tokens[1], "//") != -1)
    {
      parsedOK = parseFaceVertexNormal(_tokens);
    }
    // if we have 1 / it is a VertUV format
    else if (ps::count(_tokens[1], "/") == 1)
    {
      parsedOK = parseFaceVertexUV(_tokens);
    }
    m_faceCount += 3;
  }
  // we have a quad!
  // ngl::Face f1; // index 1 2 3
  // ngl::Face f2; // index 1 3 4
  if (_tokens.size() == 5)
  {
    // std::cout << "found quad\n";
    // for (auto t : _tokens)
    //   std::cout << t << ' ';
    // std::cout << '\n';
    std::vector<std::string> tokens[2];

    tokens[0].push_back(_tokens[0]);
    tokens[0].push_back(_tokens[1]);
    tokens[0].push_back(_tokens[2]);
    tokens[0].push_back(_tokens[3]);
    tokens[1].push_back(_tokens[0]);
    tokens[1].push_back(_tokens[1]);
    tokens[1].push_back(_tokens[3]);
    tokens[1].push_back(_tokens[4]);
    // int i = 1;
    for (int i = 0; i < 2; ++i)
    {
      if (ps::count(tokens[i][1], "/") == 2 && ps::find(tokens[i][1], "//") == -1)
      {
        // /std::cout << tokens[i][0] << " " << tokens[i][1] << " " << tokens[i][2] << '\n';
        parsedOK = parseFaceVertexNormalUV(tokens[i]);
      }

      else if (ps::find(tokens[i][1], "/") == -1)
      {
        parsedOK = parseFaceVertex(tokens[i]);
      }
      // look for VertNormal
      else if (ps::find(tokens[i][1], "//") != -1)
      {
        parsedOK = parseFaceVertexNormal(tokens[i]);
      }
      // if we have 1 / it is a VertUV format
      else if (ps::count(tokens[i][1], "/") == 1)
      {
        parsedOK = parseFaceVertexUV(tokens[i]);
      }
    }
    m_faceCount += 6;
  }

  return parsedOK;
}

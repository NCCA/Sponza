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

void GroupedObj::save(const std::string &_fname) const
{
}
bool GroupedObj::saveBinary(const std::string &_fname)
{
}
bool GroupedObj::loadBinary(const std::string &_fname)
{
}
std::vector<MeshData>::const_iterator GroupedObj::begin() const
{
  return m_meshes.begin();
}
std::vector<MeshData>::const_iterator GroupedObj::end() const
{
  return m_meshes.end();
}
std::vector<MeshData>::iterator GroupedObj::begin()
{
  return m_meshes.begin();
}

std::vector<MeshData>::iterator GroupedObj::end()
{
  return m_meshes.end();
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

/*

#include <ngl/VAOFactory.h>
#include <VAO.h>
//----------------------------------------------------------------------------------------------------------------------
/// @file GroupedObj.cpp

//----------------------------------------------------------------------------------------------------------------------

// make a namespace for our parser to save writing boost::spirit:: all the time

bool Obj::parseVertex(std::vector<std::string> &_tokens) noexcept
{
  bool parsedOK = true;
  try
  {
    float x = std::stof(_tokens[1]);
    float y = std::stof(_tokens[2]);
    float z = std::stof(_tokens[3]);
    m_verts.push_back({x, y, z});
    ++m_currentVertexOffset;
  }
  catch (std::invalid_argument &arg)
  {
    NGLMessage::addError("problem converting Obj file vertex ");
    NGLMessage::addError(arg.what());
    parsedOK = false;
  }
  return parsedOK;
}

void GroupedObj::parseMaterial(const char *_begin)
{
  // set current group
  std::vector<std::string> material;
  srule grouping = "usemtl" >> *(spt::lexeme_d[spt::anychar_p >> *(spt::anychar_p)][spt::append(material)]);

  spt::parse_info<> result = parse(_begin, grouping, spt::space_p);
  // std::cout<<"parsing material "<<_begin<<"\n";
  NGL_UNUSED(result);
  if (material.empty())
  {
    m_currentMaterial = "default";
  }
  else
  {
    m_currentMaterial = material[0];
    boost::trim(m_currentMaterial);
  }
  // std::cout<<"current material "<<m_currentMaterial<<"\n";
}

void GroupedObj::parseGroup(const char *_begin)
{

  // as the group is defined then the face data follows
  // we need to load the first set of faces before saving the data
  // this static int does that
  static int firstTime = 0;

  std::cout << "current group " << m_currentMeshName << "\n";
  if (firstTime++ > 0)
  {
    // now we add the group to our list
    m_currentMesh.m_material = m_currentMaterial;
    m_currentMesh.m_name = m_currentMeshName;
    m_currentMesh.m_numVerts = m_faceCount;
    // index into the VAO data 3 tris with uv, normal and x,y,z as floats
    m_currentMesh.m_startIndex = m_offset;
    std::cout << "Stored values fc " << m_faceCount << "\n";
    std::cout << "offset " << m_offset << "\n";
    m_meshes.push_back(m_currentMesh);

    m_offset += m_faceCount;
    m_faceCount = 0;
  }

  // set current group
  std::vector<std::string> group;
  srule grouping = "g" >> *(spt::lexeme_d[spt::anychar_p >> *(spt::anychar_p)][spt::append(group)]);

  spt::parse_info<> result = parse(_begin, grouping, spt::space_p);
  NGL_UNUSED(result);
  if (group.empty())
  {
    m_currentMeshName = "none";
  }
  else
  {
    m_currentMeshName = group[0];
  }
}

//----------------------------------------------------------------------------------------------------------------------
// parse a texture coordinate
void GroupedObj::parseTextureCoordinate(const char *_begin)
{
  std::vector<float> values;
  // generate our parse rule for a tex cord,
  // this can be either a 2 or 3 d text so the *rule looks for an additional one
  srule texcord = "vt" >> spt::real_p[spt::append(values)] >>
                  spt::real_p[spt::append(values)] >>
                  *(spt::real_p[spt::append(values)]);
  spt::parse_info<> result = spt::parse(_begin, texcord, spt::space_p);
  // should check the return values at some stage
  NGL_UNUSED(result);

  // build tex cord
  // if we have a value use it other wise set to 0
  float vt3 = values.size() == 3 ? values[2] : 0.0f;
  m_uv.push_back(ngl::Vec3(values[0], values[1], vt3));
}

//----------------------------------------------------------------------------------------------------------------------
// parse a normal
void GroupedObj::parseNormal(const char *_begin)
{
  std::vector<float> values;
  // here is our rule for normals
  srule norm = "vn" >> spt::real_p[spt::append(values)] >>
               spt::real_p[spt::append(values)] >>
               spt::real_p[spt::append(values)];
  // parse and push back to the list
  spt::parse_info<> result = spt::parse(_begin, norm, spt::space_p);
  // should check the return values at some stage
  NGL_UNUSED(result);
  m_norm.push_back(ngl::Vec3(values[0], values[1], values[2]));
}

void GroupedObj::splitFace(const std::vector<unsigned int> &_v, const std::vector<unsigned int> &_t, const std::vector<unsigned int> &_n)
{
  // std::cout<< "splitting quad face to triangles \n";
  //  so now build a face structure.
  ngl::Face f1; // index 0 1 2
  ngl::Face f2; // index 0 2 3

  // verts are -1 the size
  f1.m_numVerts = 2;
  f1.m_textureCoord = false;
  f2.m_numVerts = 2;
  f2.m_textureCoord = false;
  // -1 as the index in obj is one less
  f1.m_vert.push_back(_v[0] - 1);
  f1.m_vert.push_back(_v[1] - 1);
  f1.m_vert.push_back(_v[2] - 1);
  f2.m_vert.push_back(_v[0] - 1);
  f2.m_vert.push_back(_v[2] - 1);
  f2.m_vert.push_back(_v[3] - 1);

  // merge in texture coordinates and normals, if present
  // OBJ format requires an encoding for faces which uses one of the vertex/texture/normal specifications
  // consistently across the entire face.  eg. we can have all v/vt/vn, or all v//vn, or all v, but not
  // v//vn then v/vt/vn ...
  if (!_n.empty())
  {
    if (_n.size() != _v.size())
    {
      std::cerr << "Something wrong with the face data will continue but may not be correct\n";
    }

    f1.m_norm.push_back(_n[0] - 1);
    f1.m_norm.push_back(_n[1] - 1);
    f1.m_norm.push_back(_n[2] - 1);
    f2.m_norm.push_back(_n[0] - 1);
    f2.m_norm.push_back(_n[2] - 1);
    f2.m_norm.push_back(_n[3] - 1);
  }

  //
  // merge in texture coordinates, if present
  //
  if (!_t.empty())
  {
    if (_t.size() != _v.size())
    {
      std::cerr << "Something wrong with the face data will continue but may not be correct\n";
    }

    // copy in these references to normal vectors to the mesh's normal vector

    f1.m_uv.push_back(_t[0] - 1);
    f1.m_uv.push_back(_t[1] - 1);
    f1.m_uv.push_back(_t[2] - 1);
    f2.m_uv.push_back(_t[0] - 1);
    f2.m_uv.push_back(_t[2] - 1);
    f2.m_uv.push_back(_t[3] - 1);

    f1.m_textureCoord = true;
    f2.m_textureCoord = true;
  }
  // finally save the face into our face list
  m_face.push_back(f1);
  m_face.push_back(f2);
  m_faceCount += 6;
}

//----------------------------------------------------------------------------------------------------------------------
// parse face
void GroupedObj::parseFace(const char *_begin)
{
  // ok this one is quite complex first create some lists for our face data
  // list to hold the vertex data indices
  std::vector<unsigned int> vec;
  // list to hold the tex cord indices
  std::vector<unsigned int> tvec;
  // list to hold the normal indices
  std::vector<unsigned int> nvec;

  // create the parse rule for a face entry V/T/N
  // so our entry can be always a vert, followed by optional t and norm seperated by /
  // also it is possible to have just a V value with no / so the rule should do all this
  srule entry = spt::int_p[spt::append(vec)] >>
                (("/" >> (spt::int_p[spt::append(tvec)] | spt::epsilon_p) >>
                  "/" >> (spt::int_p[spt::append(nvec)] | spt::epsilon_p)) |
                 spt::epsilon_p);
  // a face has at least 3 of the above entries plus many optional ones
  srule face = "f" >> entry >> entry >> entry >> *(entry);
  // now we've done this we can parse
  spt::parse(_begin, face, spt::space_p);

  size_t numVerts = vec.size();
  if (numVerts != 3)
  {
    // std::cerr<<"Warning non-triangular face if quad will split else re-model. size is "<<numVerts<<"\n";
    if (numVerts == 4)
      splitFace(vec, tvec, nvec);
  }
  else
  {
    // so now build a face structure.
    ngl::Face f;
    // verts are -1 the size
    f.m_numVerts = numVerts - 1;
    f.m_textureCoord = false;
    // copy the vertex indices into our face data structure index in obj start from 1
    // so we need to do -1 for our array index
    for (auto i : vec)
    {
      f.m_vert.push_back(i - 1);
    }

    // merge in texture coordinates and normals, if present
    // OBJ format requires an encoding for faces which uses one of the vertex/texture/normal specifications
    // consistently across the entire face.  eg. we can have all v/vt/vn, or all v//vn, or all v, but not
    // v//vn then v/vt/vn ...
    if (!nvec.empty())
    {
      if (nvec.size() != vec.size())
      {
        std::cerr << "Something wrong with the face data will continue but may not be correct\n";
      }

      // copy in these references to normal vectors to the mesh's normal vector
      for (auto i : nvec)
      {
        f.m_norm.push_back(i - 1);
      }
    }

    //
    // merge in texture coordinates, if present
    //
    if (!tvec.empty())
    {
      if (tvec.size() != vec.size())
      {
        std::cerr << "Something wrong with the face data will continue but may not be correct\n";
      }

      // copy in these references to normal vectors to the mesh's normal vector
      for (auto i : tvec)
      {
        f.m_uv.push_back(i - 1);
      }

      f.m_textureCoord = true;
    }
    // finally save the face into our face list
    m_face.push_back(f);
    m_faceCount += 3;
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool GroupedObj::load(const std::string &_fname, CalcBB _calcBB) noexcept
{
  std::ifstream in(_fname.data());
  if (in.is_open() != true)
  {
    NGLMessage::addError(fmt::format(" file {0} not found  ", _fname.data()));
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

//----------------------------------------------------------------------------------------------------------------------
GroupedObj::GroupedObj(const std::string &_fname) : ngl::AbstractMesh()
{
  m_vbo = false;
  m_ext = nullptr;
  // set default values
  // m_nVerts=m_nNorm=m_nTex=m_nFaces=0;
  // set the default extents to 0
  m_maxX = 0.0f;
  m_maxY = 0.0f;
  m_maxZ = 0.0f;
  m_minX = 0.0f;
  m_minY = 0.0f;
  m_minZ = 0.0f;
  m_offset = 0;

  // load the file in

  //    m_uvture = false;
  m_faceCount = 0;
  m_currentMesh.m_startIndex = 0;
  m_currentMesh.m_numVerts = 0;

  m_loaded = load(_fname);
  // as the face triggers the push back of the meshes once we have finished the load we need to add the rest
  m_currentMesh.m_material = m_currentMaterial;
  m_currentMesh.m_name = m_currentMeshName;
  m_currentMesh.m_numVerts = m_faceCount;
  // index into the VAO data 3 tris with uv, normal and x,y,z as floats
  m_currentMesh.m_startIndex = m_offset;
  m_meshes.push_back(m_currentMesh);
  std::sort(m_meshes.begin(), m_meshes.end());
  // now create the VAO
  createVAO();
}

//----------------------------------------------------------------------------------------------------------------------
void GroupedObj::save(const std::string &_fname) const
{
  // Open the stream and parse
  std::fstream fileOut;
  fileOut.open(_fname.c_str(), std::ios::out);
  if (!fileOut.is_open())
  {
    std::cout << "File : " << _fname << " Not founds " << std::endl;
    return;
  }
  // write out some comments
  fileOut << "# This file was created by ngl Obj exporter " << _fname.c_str() << std::endl;
  // was c++ 11  for(ngl::Vec3 v : m_norm) for all of these
  // write out the verts
  for (auto v : m_verts)
  {
    fileOut << "v " << v.m_x << " " << v.m_y << " " << v.m_z << std::endl;
  }

  // write out the tex cords
  for (auto v : m_uv)
  {
    fileOut << "vt " << v.m_x << " " << v.m_y << std::endl;
  }
  // write out the normals

  for (auto v : m_norm)
  {
    fileOut << "vn " << v.m_x << " " << v.m_y << " " << v.m_z << std::endl;
  }

  // finally the faces
  for (auto f : m_face)
  {
    fileOut << "f ";
    // we now have V/T/N for each to write out
    for (size_t i = 0; i < f.m_numVerts; ++i)
    {
      // don't forget that obj indices start from 1 not 0 (i did originally !)
      fileOut << f.m_vert[i] + 1;
      fileOut << "/";
      fileOut << f.m_uv[i] + 1;
      fileOut << "/";

      fileOut << f.m_norm[i] + 1;
      fileOut << " ";
    }
    fileOut << '\n';
  }
}

//----------------------------------------------------------------------------------------------------------------------

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
  //  std::cout<<"Drawing \n";
  m_vaoMesh->bind();

  reinterpret_cast<VAO *>(m_vaoMesh.get())->draw(m_meshes[_meshID].m_startIndex, m_meshes[_meshID].m_numVerts);

  m_vaoMesh->unbind();
}

// first write out the mesh data m_meshes.size() then for each
// length std::string m_name;
// lenght std::string m_material;
//  unsigned int m_startIndex;
//  unsigned int m_numVerts;

bool GroupedObj::saveBinary(const std::string &_fname)
{
  std::ofstream fileOut;
  fileOut.open(_fname.c_str(), std::ios::out | std::ios::binary);
  if (!fileOut.is_open())
  {
    std::cout << "File : " << _fname << " could not be written for output\n";
    return false;
  }

  // write our own id into the file so we can check we have the correct type
  // when loading
  const std::string header("ngl::objbin");
  fileOut.write(header.c_str(), header.length());

  size_t size = m_meshes.size();
  fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
  for (auto m : m_meshes)
  {
    size = m.m_name.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(m.m_name.c_str()), size);
    size = m.m_material.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(m.m_material.c_str()), size);
    fileOut.write(reinterpret_cast<char *>(&m.m_startIndex), sizeof(unsigned int));
    fileOut.write(reinterpret_cast<char *>(&m.m_numVerts), sizeof(unsigned int));
  }

  m_vaoMesh->bind();

  size = reinterpret_cast<VAO *>(m_vaoMesh.get())->getSize();

  fileOut.write(reinterpret_cast<char *>(&size), sizeof(unsigned int));

  ngl::Real *vboMem = this->mapVAOVerts();
  fileOut.write(reinterpret_cast<char *>(vboMem), size);
  this->unMapVAO();

  fileOut.close();
  return true;
}

// a simple structure to hold our vertex data
struct vertData
{
  GLfloat x; // position from obj
  GLfloat y;
  GLfloat z;
  GLfloat nx; // normal from obj mesh
  GLfloat ny;
  GLfloat nz;
  GLfloat u; // tex cords from obj
  GLfloat v; // tex cords
};

bool GroupedObj::loadBinary(const std::string &_fname)
{
  m_meshes.clear();
  std::ifstream fileIn;
  fileIn.open(_fname.c_str(), std::ios::in | std::ios::binary);
  if (!fileIn.is_open())
  {
    std::cout << "File : " << _fname << " could not be opened for reading" << std::endl;
    return false;
  }
  char header[12];
  fileIn.read(header, 11 * sizeof(char));
  header[11] = 0; // for strcmp we need \n
  // basically I used the magick string ngl::bin (I presume unique in files!) and
  // we test against it.
  if (strcmp(header, "ngl::objbin"))
  {
    // best close the file and exit
    fileIn.close();
    std::cout << "this is not an ngl::objbin file " << std::endl;
    return false;
  }

  unsigned int vecsize;
  fileIn.read(reinterpret_cast<char *>(&vecsize), sizeof(vecsize));
  unsigned int size;
  std::string s;
  for (size_t i = 0; i < vecsize; ++i)
  {
    MeshData d;
    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    d.m_name = s;
    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    d.m_material = s;
    fileIn.read(reinterpret_cast<char *>(&d.m_startIndex), sizeof(unsigned int));
    fileIn.read(reinterpret_cast<char *>(&d.m_numVerts), sizeof(unsigned int));
    m_meshes.push_back(d);
  }

  fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
  std::unique_ptr<ngl::Real[]> data(new ngl::Real[size]);
  fileIn.read(reinterpret_cast<char *>(&data[0]), size);
  fileIn.close();

  // first we grab an instance of our VOA
  m_vaoMesh = ngl::VAOFactory::createVAO("sponzaVAO", GL_TRIANGLES);
  // next we bind it so it's active for setting data
  m_vaoMesh->bind();
  m_meshSize = size;

  // now we have our data add it to the VAO, we need to tell the VAO the following
  // how much (in bytes) data we are copying
  // a pointer to the first element of data (in this case the address of the first element of the
  // std::vector
  m_vaoMesh->setData(VAO::VertexData(size, data[0]));
  // in this case we have packed our data in interleaved format as follows
  // u,v,nx,ny,nz,x,y,z
  // If you look at the shader we have the following attributes being used
  // attribute vec3 inVert; attribute 0
  // attribute vec2 inUV; attribute 1
  // attribute vec3 inNormal; attribure 2
  // so we need to set the vertexAttributePointer so the correct size and type as follows
  // vertex is attribute 0 with x,y,z(3) parts of type GL_FLOAT, our complete packed data is
  // sizeof(vertData) and the offset into the data structure for the first x component is 5 (u,v,nx,ny,nz)..x
  // a simple structure to hold our vertex data

  m_vaoMesh->setVertexAttributePointer(0, 3, GL_FLOAT, sizeof(vertData), 0);
  // uv same as above but starts at 0 and is attrib 1 and only u,v so 2
  m_vaoMesh->setVertexAttributePointer(1, 3, GL_FLOAT, sizeof(vertData), 3);
  // normal same as vertex only starts at position 2 (u,v)-> nx
  m_vaoMesh->setVertexAttributePointer(2, 2, GL_FLOAT, sizeof(vertData), 6);

  // now we have set the vertex attributes we tell the VAO class how many indices to draw when
  // glDrawArrays is called, in this case we use buffSize (but if we wished less of the sphere to be drawn we could
  // specify less (in steps of 3))
  m_vaoMesh->setNumIndices(size / sizeof(vertData));
  // finally we have finished for now so time to unbind the VAO
  m_vaoMesh->unbind();

  // indicate we have a vao now
  m_vao = true;
  return true;
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
  std::vector<vertData> vboMesh;
  vertData d;
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
  m_vaoMesh->setData(VAO::VertexData(m_meshSize * sizeof(vertData), vboMesh[0].x));
  // in this case we have packed our data in interleaved format as follows
  // u,v,nx,ny,nz,x,y,z
  // If you look at the shader we have the following attributes being used
  // attribute vec3 inVert; attribute 0
  // attribute vec2 inUV; attribute 1
  // attribute vec3 inNormal; attribure 2
  // so we need to set the vertexAttributePointer so the correct size and type as follows
  // vertex is attribute 0 with x,y,z(3) parts of type GL_FLOAT, our complete packed data is
  // sizeof(vertData) and the offset into the data structure for the first x component is 5 (u,v,nx,ny,nz)..x
  m_vaoMesh->setVertexAttributePointer(0, 3, GL_FLOAT, sizeof(vertData), 0);
  // uv same as above but starts at 0 and is attrib 1 and only u,v so 2
  m_vaoMesh->setVertexAttributePointer(1, 3, GL_FLOAT, sizeof(vertData), 3);
  // normal same as vertex only starts at position 2 (u,v)-> nx
  m_vaoMesh->setVertexAttributePointer(2, 2, GL_FLOAT, sizeof(vertData), 6);

  // now we have set the vertex attributes we tell the VAO class how many indices to draw when
  // glDrawArrays is called, in this case we use buffSize (but if we wished less of the sphere to be drawn we could
  // specify less (in steps of 3))
  m_vaoMesh->setNumIndices(m_meshSize);
  // finally we have finished for now so time to unbind the VAO
  m_vaoMesh->unbind();

  // indicate we have a vao now
  m_vao = true;
}
*/
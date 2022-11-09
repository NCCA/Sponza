#include "Mtl.h"
#include <fstream>
#include <ngl/NGLStream.h>
#include <ngl/Texture.h>
#include <ngl/ShaderLib.h>
#include <list>
#include <ngl/pystring.h>

namespace ps = pystring;

bool Mtl::load(const std::string &_fname)
{
  std::fstream fileIn;
  fileIn.open(_fname.c_str(), std::ios::in);
  if (!fileIn.is_open())
  {
    std::cout << "File : " << _fname << " Not found Exiting " << std::endl;
    return false;
  }

  // this is the line we wish to parse
  std::string lineBuffer;
  // say which separators should be used in this
  // case Spaces, Tabs and return \ new line
  std::vector<std::string> tokens;

  // loop through the file
  while (!fileIn.eof())
  {
    // grab a line from the input
    getline(fileIn, lineBuffer, '\n');
    // make sure it's not an empty line
    if (lineBuffer.size() > 1)
    {
      // now tokenize the line
      ps::split(lineBuffer, tokens);
      // and get the first token
      // now see if it's a valid one and call the correct function
      if (tokens[0] == "newmtl")
      {

        // add to our map it is possible that a badly formed file would not have an mtl
        // def first however this is so unlikely I can't be arsed to handle that case.
        // If it does crash it could be due to this code.
        // std::cout<<"found "<<m_currentName<<"\n";
        m_currentName = tokens[1];
        m_current = new mtlItem;
        // These are the OpenGL texture ID's so set to zero first (for no texture)
        m_current->map_KaId = 0;
        m_current->map_KdId = 0;
        m_current->map_dId = 0;
        m_current->map_bumpId = 0;
        m_current->bumpId = 0;

        m_materials[m_currentName] = m_current;
      }
      else if (tokens[0] == "Ns")
      {
        m_current->Ns = std::stof(tokens[1]);
      }
      else if (tokens[0] == "Ni")
      {
        m_current->Ni = std::stof(tokens[1]);
      }
      else if (tokens[0] == "d")
      {
        m_current->d = std::stof(tokens[1]);
      }
      else if (tokens[0] == "Tr")
      {
        m_current->Tr = std::stof(tokens[1]);
      }
      else if (tokens[0] == "Tf")
      {
        m_current->Tf.m_x = std::stof(tokens[1]);
        m_current->Tf.m_y = std::stof(tokens[2]);
        m_current->Tf.m_z = std::stof(tokens[3]);
      }
      else if (tokens[0] == "illum")
      {
        m_current->illum = std::stoi(tokens[1]);
      }
      else if (tokens[0] == "Ka")
      {
        m_current->Ka.m_x = std::stof(tokens[1]);
        m_current->Ka.m_y = std::stof(tokens[2]);
        m_current->Ka.m_z = std::stof(tokens[3]);
      }
      else if (tokens[0] == "Kd")
      {
        m_current->Kd.m_x = std::stof(tokens[1]);
        m_current->Kd.m_y = std::stof(tokens[2]);
        m_current->Kd.m_z = std::stof(tokens[3]);
      }
      else if (tokens[0] == "Ks")
      {
        m_current->Ks.m_x = std::stof(tokens[1]);
        m_current->Ks.m_y = std::stof(tokens[2]);
        m_current->Ks.m_z = std::stof(tokens[3]);
      }
      else if (tokens[0] == "Ke")
      {
        m_current->Ke.m_x = std::stof(tokens[1]);
        m_current->Ke.m_y = std::stof(tokens[2]);
        m_current->Ke.m_z = std::stof(tokens[3]);
      }

      else if (tokens[0] == "map_Ka")
      {
        m_current->map_Ka = convertToPath(tokens[1]);
      }
      else if (tokens[0] == "map_Kd")
      {
        m_current->map_Kd = convertToPath(tokens[1]);
      }
      else if (tokens[0] == "map_d")
      {
        m_current->map_d = convertToPath(tokens[1]);
      }
      else if (tokens[0] == "map_bump")
      {
        m_current->map_bump = convertToPath(tokens[1]);
      }
      else if (tokens[0] == "bump")
      {
        m_current->bump = convertToPath(tokens[1]);
      }
      else if (tokens[0] == "map_Ks")
      {
        m_current->map_Ks = convertToPath(tokens[1]);
      }
    } // end zero line
  }   // end while

  // as the trigger for putting the meshes back is the newmtl we will always have a hanging one
  // this adds it to the list
  m_materials[m_currentName] = m_current;
  loadTextures();
  return true;
}

Mtl::Mtl(const std::string &_fname, bool _loadTextures)
{
  m_loadTextures = _loadTextures;
  load(_fname);
  if (m_loadTextures == true)
  {
    loadTextures();
  }
}

Mtl::~Mtl()
{
  clear();
}

void Mtl::loadTextures()
{
  m_textureID.clear();
  std::cout << "loading textures this may take some time\n";
  // first loop and store all the texture names in the container
  std::list<std::string> names;
  auto end = m_materials.end();
  auto i = m_materials.begin();
  for (; i != end; ++i)
  {
    if (i->second->map_Ka.size() != 0)
      names.push_back(i->second->map_Ka);
    if (i->second->map_Kd.size() != 0)
      names.push_back(i->second->map_Kd);
    if (i->second->map_d.size() != 0)
      names.push_back(i->second->map_d);
    if (i->second->map_bump.size() != 0)
      names.push_back(i->second->map_bump);
    if (i->second->map_bump.size() != 0)
      names.push_back(i->second->bump);
  }

  std::cout << "we have this many textures " << names.size() << "\n";
  // now remove duplicates
  names.unique();
  std::cout << "we have " << names.size() << " unique textures to load\n";
  // now we load the textures and get the GL id
  // now we associate the ID with the mtlItem

  for (auto name : names)
  {
    std::cout << "loading texture " << name << "\n";
    ngl::Texture t(name);
    std::cout << t.getWidth() << " x " << t.getHeight() << '\n';
    GLuint textureID = t.setTextureGL();
    m_textureID.push_back(textureID);
    std::cout << "processing " << name << " ID" << textureID << "\n";
    i = m_materials.begin();
    for (; i != end; ++i)
    {
      if (i->second->map_Ka == name)
        i->second->map_KaId = textureID;
      if (i->second->map_Kd == name)
        i->second->map_KdId = textureID;
      if (i->second->map_d == name)
        i->second->map_dId = textureID;
      if (i->second->map_bump == name)
        i->second->map_bumpId = textureID;
      if (i->second->bump == name)
        i->second->bumpId = textureID;
    }
  }

  std::cout << "done \n";
}

void Mtl::clear()
{
  auto end = m_materials.end();
  auto i = m_materials.begin();
  for (; i != end; ++i)
  {
    delete i->second;
  }
  if (m_loadTextures == true)
  {
    for (auto i : m_textureID)
      glDeleteTextures(1, &i);
  }
}

std::string Mtl::convertToPath(std::string _p) const
{

  ps::strip(_p, " \t\n\r");
#ifdef WIN32
  _p = ps::replace(_p, "\\", "\\\\");
#else
  _p = ps::replace(_p, "\\", "/");

#endif
  return _p;
}

void Mtl::debugPrint() const
{
  auto end = m_materials.end();
  auto i = m_materials.begin();
  std::cout << m_materials.size() << "\n";
  for (; i != end; ++i)
  {
    std::cerr << "-------------------------------------------------\n";
    std::cout << "Material Name " << i->first << "\n";
    std::cerr << "-------------------------------------------------\n";
    std::cout << "Ns " << i->second->Ns << "\n";
    std::cout << "Ni " << i->second->Ni << "\n";
    std::cout << "d " << i->second->d << "\n";
    std::cout << "Tr " << i->second->Tr << "\n";
    std::cout << "illum " << i->second->illum << "\n";
    std::cout << "Tf " << i->second->Tf << "\n";
    std::cout << "Ka " << i->second->Ka << "\n";
    std::cout << "Kd " << i->second->Kd << "\n";
    std::cout << "Ks " << i->second->Ks << "\n";
    std::cout << "Ke " << i->second->Ke << "\n";
    std::cout << "map_Ka " << i->second->map_Ka << "\n";
    std::cout << "map_Kd " << i->second->map_Kd << "\n";
    std::cout << "map_d " << i->second->map_d << "\n";
    std::cout << "map_bump " << i->second->map_bump << "\n";
    std::cout << "bump " << i->second->bump << "\n";
    std::cout << "map_Ka Texture ID" << i->second->map_KaId << "\n";
    std::cout << "map_Kd Texture ID " << i->second->map_KdId << "\n";
    std::cout << "map_d Texture ID " << i->second->map_dId << "\n";
    std::cout << "map_bump Texture ID " << i->second->map_bumpId << "\n";
    std::cout << "bump Texture ID " << i->second->bumpId << "\n";
    std::cerr << "-------------------------------------------------\n";
  }
}

mtlItem *Mtl::find(const std::string &_n) const
{
  auto material = m_materials.find(_n);
  // make sure we have a valid  material
  if (material != m_materials.end())
  {
    return material->second;
  }
  else
  {
    std::cerr << "Warning could not find material " << _n << "\n";
    return 0;
  }
}

bool Mtl::saveBinary(const std::string &_fname) const
{
  std::ofstream fileOut;
  fileOut.open(_fname.c_str(), std::ios::out | std::ios::binary);
  if (!fileOut.is_open())
  {
    std::cout << "File : " << _fname << " could not be written for output" << std::endl;
    return false;
  }
  // write our own id into the file so we can check we have the correct type
  // when loading
  const std::string header("ngl::mtlbin");
  fileOut.write(header.c_str(), header.length());

  unsigned int size = m_materials.size();
  fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
  auto start = m_materials.begin();
  auto end = m_materials.end();
  for (; start != end; ++start)
  {
    // std::cout<<"writing out "<<start->first<<"\n";
    //  first write the length of the string
    size = start->first.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->first.c_str()), size);
    // now we do the different data elements of the mtlItem.
    fileOut.write(reinterpret_cast<char *>(&start->second->Ns), sizeof(float));
    fileOut.write(reinterpret_cast<char *>(&start->second->Ni), sizeof(float));
    fileOut.write(reinterpret_cast<char *>(&start->second->d), sizeof(float));
    fileOut.write(reinterpret_cast<char *>(&start->second->Tr), sizeof(float));
    fileOut.write(reinterpret_cast<char *>(&start->second->illum), sizeof(int));

    fileOut.write(reinterpret_cast<char *>(&start->second->Tf), sizeof(ngl::Vec3));
    fileOut.write(reinterpret_cast<char *>(&start->second->Ka), sizeof(ngl::Vec3));
    fileOut.write(reinterpret_cast<char *>(&start->second->Kd), sizeof(ngl::Vec3));
    fileOut.write(reinterpret_cast<char *>(&start->second->Ks), sizeof(ngl::Vec3));
    fileOut.write(reinterpret_cast<char *>(&start->second->Ke), sizeof(ngl::Vec3));

    // first write the length of the string
    size = start->second->map_Ka.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->second->map_Ka.c_str()), size);

    // first write the length of the string
    size = start->second->map_Kd.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->second->map_Kd.c_str()), size);

    // first write the length of the string
    size = start->second->map_d.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->second->map_d.c_str()), size);

    // first write the length of the string
    size = start->second->map_bump.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->second->map_bump.c_str()), size);

    // first write the length of the string
    size = start->second->bump.length();
    fileOut.write(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string
    fileOut.write(reinterpret_cast<const char *>(start->second->bump.c_str()), size);
  }

  fileOut.close();
  return true;
}

bool Mtl::loadBinary(const std::string &_fname)
{
  std::ifstream fileIn;
  fileIn.open(_fname.c_str(), std::ios::in | std::ios::binary);
  if (!fileIn.is_open())
  {
    std::cout << "File : " << _fname << " could not be opened for reading" << std::endl;
    return false;
  }
  // clear out what we already have.
  clear();
  unsigned int mapsize;

  char header[12];
  fileIn.read(header, 11 * sizeof(char));
  header[11] = 0; // for strcmp we need \n
  // basically I used the magick string ngl::bin (I presume unique in files!) and
  // we test against it.
  if (strcmp(header, "ngl::mtlbin"))
  {
    // best close the file and exit
    fileIn.close();
    std::cout << "this is not an ngl::mtlbin file " << std::endl;
    return false;
  }

  fileIn.read(reinterpret_cast<char *>(&mapsize), sizeof(mapsize));
  unsigned int size;
  std::string materialName;
  std::string s;
  for (unsigned int i = 0; i < mapsize; ++i)
  {
    mtlItem *item = new mtlItem;

    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    materialName.resize(size);
    fileIn.read(reinterpret_cast<char *>(&materialName[0]), size);
    // now we do the different data elements of the mtlItem.
    fileIn.read(reinterpret_cast<char *>(&item->Ns), sizeof(float));
    fileIn.read(reinterpret_cast<char *>(&item->Ni), sizeof(float));
    fileIn.read(reinterpret_cast<char *>(&item->d), sizeof(float));
    fileIn.read(reinterpret_cast<char *>(&item->Tr), sizeof(float));
    fileIn.read(reinterpret_cast<char *>(&item->illum), sizeof(int));

    fileIn.read(reinterpret_cast<char *>(&item->Tf), sizeof(ngl::Vec3));
    fileIn.read(reinterpret_cast<char *>(&item->Ka), sizeof(ngl::Vec3));
    fileIn.read(reinterpret_cast<char *>(&item->Kd), sizeof(ngl::Vec3));
    fileIn.read(reinterpret_cast<char *>(&item->Ks), sizeof(ngl::Vec3));
    fileIn.read(reinterpret_cast<char *>(&item->Ke), sizeof(ngl::Vec3));
    // more strings
    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    item->map_Ka = s;

    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    item->map_Kd = s;

    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    item->map_d = s;

    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    item->map_bump = s;

    fileIn.read(reinterpret_cast<char *>(&size), sizeof(size));
    // now the string we first need to allocate space then copy in
    s.resize(size);
    fileIn.read(reinterpret_cast<char *>(&s[0]), size);
    item->bump = s;

    m_materials[materialName] = item;
  }
  m_loadTextures = true;
  loadTextures();
  return true;
}

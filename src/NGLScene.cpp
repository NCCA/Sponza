#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOFactory.h>
#include "VAO.h"

NGLScene::NGLScene()
{
  setTitle("Sponza Demo");
  m_whichMap = 0;
  ngl::VAOFactory::registerVAOCreator("sponzaVAO", VAO::create);
  ngl::Image::info();
}

NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(int _w, int _h)
{
  m_project = ngl::perspective(45.0f, static_cast<float>(_w) / _h, 0.5f, 3550.0f);
  m_win.width = static_cast<int>(_w * devicePixelRatio());
  m_win.height = static_cast<int>(_h * devicePixelRatio());
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  ngl::Vec3 from(0, 40, -140);
  ngl::Vec3 to(0, 40, 0);
  ngl::Vec3 up(0, 1, 0);
  m_view = ngl::lookAt(from, to, up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project = ngl::perspective(50, 1024.0f / 720.0f, 0.01, 200.0);
  // load a frag and vert shaders

  ngl::ShaderLib::createShaderProgram("TextureShader");

  ngl::ShaderLib::attachShader("TextureVertex", ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("TextureFragment", ngl::ShaderType::FRAGMENT);
  ngl::ShaderLib::loadShaderSource("TextureVertex", "shaders/TextureVert.glsl");
  ngl::ShaderLib::loadShaderSource("TextureFragment", "shaders/TextureFrag.glsl");

  ngl::ShaderLib::compileShader("TextureVertex");
  ngl::ShaderLib::compileShader("TextureFragment");
  ngl::ShaderLib::attachShaderToProgram("TextureShader", "TextureVertex");
  ngl::ShaderLib::attachShaderToProgram("TextureShader", "TextureFragment");
  // bind our attributes for the vertex shader

  // link the shader no attributes are bound
  ngl::ShaderLib::linkProgramObject("TextureShader");
  ngl::ShaderLib::use("TextureShader");

  glEnable(GL_DEPTH_TEST);

  m_mtl.reset(new Mtl);
  bool loaded = m_mtl->load("models/sponza.mtl");

  if (loaded == false)
  {
    std::cerr << "error loading mtl file ";
    exit(EXIT_FAILURE);
  }

  m_model.reset(new GroupedObj("models/sponza.obj"));
  if (loaded == false)
  {
    std::cerr << "error loading obj file ";
    exit(EXIT_FAILURE);
  }
  // as re-size is not explicitly called we need to do this.
  glViewport(0, 0, width(), height());
}

void NGLScene::loadMatricesToShader()
{

  ngl::Mat4 MVP = m_project * m_view *
                  m_mouseGlobalTX *
                  m_transform.getMatrix();

  ngl::ShaderLib::setUniform("MVP", MVP);
}

void NGLScene::paintGL()
{

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX = ngl::Mat4::rotateX(m_win.spinXFace);
  ngl::Mat4 rotY = ngl::Mat4::rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX = rotY * rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, m_win.width, m_win.height);
  loadMatricesToShader();
  auto end = m_model->numMeshes();
  std::string matName;
  for (unsigned int i = 0; i < end; ++i)
  {
    // m_mtl->use(m_model->getMaterial(i));
    mtlItem *currMaterial = m_mtl->find(m_model->getMaterial(i));
    if (currMaterial == nullptr)
      continue;
    // see if we need to switch the material or not this saves on OpenGL calls and
    // should speed things up
    if (matName != m_model->getMaterial(i))
    {
      matName = m_model->getMaterial(i);
      switch (m_whichMap)
      {
      case 0:
        glBindTexture(GL_TEXTURE_2D, currMaterial->map_KaId);
        break;
      case 1:
        glBindTexture(GL_TEXTURE_2D, currMaterial->map_KdId);
        break;
      case 2:
        glBindTexture(GL_TEXTURE_2D, currMaterial->map_bumpId);
        break;
      case 3:
        glBindTexture(GL_TEXTURE_2D, currMaterial->bumpId);
        break;
      case 4:
        glBindTexture(GL_TEXTURE_2D, currMaterial->map_dId);
        break;
      }
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      ngl::ShaderLib::setUniform("ka", currMaterial->Ka.m_x, currMaterial->Ka.m_y, currMaterial->Ka.m_z);
      ngl::ShaderLib::setUniform("transp", currMaterial->d);
    }
    m_model->draw(i);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape:
    QGuiApplication::exit(EXIT_SUCCESS);
    break;
  // turn on wirframe rendering
  case Qt::Key_W:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  // turn off wire frame
  case Qt::Key_S:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  // show full screen
  case Qt::Key_F:
    showFullScreen();
    break;
  // show windowed
  case Qt::Key_N:
    showNormal();
    break;
  default:
    break;
  case Qt::Key_1:
    m_whichMap = 0;
    break;
  case Qt::Key_2:
    m_whichMap = 1;
    break;
  case Qt::Key_3:
    m_whichMap = 2;
    break;
  case Qt::Key_4:
    m_whichMap = 3;
    break;
  case Qt::Key_5:
    m_whichMap = 4;
    break;
  }
  // finally update the GLWindow and re-draw
  // if (isExposed())
  update();
}

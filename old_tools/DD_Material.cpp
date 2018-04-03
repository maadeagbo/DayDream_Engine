#include "DD_Material.h"

void DD_Material::AddTexture(DD_Texture2D* tex, TextureType type) {
  switch (type) {
    case ALBEDO:
      m_albedo = true;
      m_textures[TextureType::ALBEDO] = tex;
      break;
    case SPECULAR:
      m_specular = true;
      m_textures[TextureType::SPECULAR] = tex;
      break;
    case NORMAL:
      m_normal = true;
      m_textures[TextureType::NORMAL] = tex;
      break;
    case ROUGH:
      m_roughness = true;
      m_textures[TextureType::ROUGH] = tex;
      break;
    case METAL:
      m_metalness = true;
      m_textures[TextureType::METAL] = tex;
      break;
    case AO:
      m_ambient = true;
      m_textures[TextureType::AO] = tex;
      break;
    case EMISSIVE:
      m_emissive = true;
      m_textures[TextureType::EMISSIVE] = tex;
      break;
    default:
      break;
  }
}

// Check if material is present, then set multiplier material if exists
void DD_Material::SetMultiplierMaterial(TextureType type) {
  switch (type) {
    case ALBEDO:
      if (m_albedo) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case SPECULAR:
      if (m_specular) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case NORMAL:
      if (m_normal) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case ROUGH:
      if (m_roughness) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case METAL:
      if (m_metalness) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case AO:
      if (m_ambient) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    case EMISSIVE:
      if (m_emissive) {
        m_matType = MaterialType::MULTIPLIER_MAT;
      }
      break;
    default:
      break;
  }
}

bool BindTexture(TextureType type, DD_Texture2D* tex) {
  bool flag = tex->Generate(tex->path.c_str());
  tex->type = type;
  return flag;
}

bool DD_Material::OpenGLBindMaterial() {
  if (!m_loaded_to_GPU) {
    bool success = false;

    if (m_albedo) {
      success =
          BindTexture(TextureType::ALBEDO, m_textures[TextureType::ALBEDO]);
      if (!success) {
        return false;
      }
    }
    if (m_specular) {
      success =
          BindTexture(TextureType::SPECULAR, m_textures[TextureType::SPECULAR]);
      if (!success) {
        return false;
      }
    }
    if (m_normal) {
      success =
          BindTexture(TextureType::NORMAL, m_textures[TextureType::NORMAL]);
      if (!success) {
        return false;
      }
    }
    if (m_roughness) {
      success = BindTexture(TextureType::ROUGH, m_textures[TextureType::ROUGH]);
      if (!success) {
        return false;
      }
    }
    if (m_metalness) {
      success = BindTexture(TextureType::METAL, m_textures[TextureType::METAL]);
      if (!success) {
        return false;
      }
    }
    if (m_ambient) {
      success = BindTexture(TextureType::AO, m_textures[TextureType::AO]);
      if (!success) {
        return false;
      }
    }
    if (m_emissive) {
      success =
          BindTexture(TextureType::EMISSIVE, m_textures[TextureType::EMISSIVE]);
      if (!success) {
        return false;
      }
    }
  }
  m_loaded_to_GPU = true;
  return true;
}

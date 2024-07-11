#include "dlpch.h"

#include "OpenGLShaderCompiler.h"

#include <fstream>

#include "Daedalus/Config/PathConfig.h"

using namespace Daedalus;

GLuint OpenGLShaderCompiler::CompileFromCode(const std::string& code, ShaderType type)
{
    const auto code_str = code.c_str();
    GLuint shader_id = glCreateShader(GetShaderGLType(type));

    glShaderSource(shader_id, 1, &code_str, 0);
    glCompileShader(shader_id);

    GLint is_compiled = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_length);

        const auto type_str = GetShaderTypeString(type);
        if (max_length > 0)
        {
            std::vector<GLchar> info_log(max_length);
            glGetShaderInfoLog(shader_id, max_length, &max_length, &info_log[0]);
            glDeleteShader(shader_id);
            std::string log_text(info_log.data());
            Log::Write(Log::Levels::Critical, Log::Categories::Renderer, type_str + " shader compilation error: " + log_text);
            throw std::runtime_error(info_log.data());
        }
        else
        {
            glDeleteShader(shader_id);
            Log::Write(Log::Levels::Critical, Log::Categories::Renderer, type_str + " shader compilation with empty error message;");
            throw std::runtime_error(type_str + " shader compilation with empty error message;");
        }

    }

    return shader_id;
}

GLuint OpenGLShaderCompiler::BuildFromBinary(const std::filesystem::path& binary_path)
{
    const auto data = Unpack(binary_path);
    const auto program_id = glCreateProgram();
    glProgramBinary(program_id, data.format, data.binary.data(), data.length);

    GLint isLinked = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, (int*)&isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &max_length);

        if (max_length > 0)
        {
            std::vector<GLchar> info_log(max_length);
            glGetProgramInfoLog(program_id, max_length, &max_length, &info_log[0]);
            glDeleteProgram(program_id);
            std::string log_text(info_log.data());
            Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Shader binary loading error message: " + log_text);
            throw std::runtime_error(info_log.data());
        }
        else
        {
            glDeleteProgram(program_id);
            Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Shader binary loading with empty error message;");
            throw std::runtime_error("Shader binary loading with empty error message;");
        }
    }

    return program_id;
}

std::vector<GLuint> OpenGLShaderCompiler::CompileMultipleShaders(
    const std::map<OpenGLShaderCompiler::ShaderType, std::string>& shaders)
{
    std::vector<GLuint> shaders_id;
    shaders_id.reserve(shaders.size());

    for (const auto& type_source : shaders)
    {
        shaders_id.push_back(CompileFromCode(type_source.second, type_source.first));
    }

    return shaders_id;
}

GLuint OpenGLShaderCompiler::Link(const std::vector<GLuint>& shaders)
{
    const auto program_id = glCreateProgram();

    for (const auto shader_id : shaders)
    {
        glAttachShader(program_id, shader_id);
    }

    glLinkProgram(program_id);

    GLint is_linked = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, (int*)&is_linked);
    if (is_linked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &max_length);
        glDeleteProgram(program_id);

        if (max_length > 0)
        {
            std::vector<GLchar> info_log(max_length);
            glGetProgramInfoLog(program_id, max_length, &max_length, &info_log[0]);
            std::string log_text(info_log.data());
            Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Shader linking error: " + log_text);
            throw std::runtime_error(info_log.data());
        }
        else
        {
            throw std::runtime_error("Shader linking error with empty error message;");
        }
    }

    for (const auto shader_id : shaders)
    {
        glDeleteShader(shader_id);
    }

    return program_id;
}

GLuint OpenGLShaderCompiler::BuildFromFile(const std::filesystem::path& source_path)
{
    const auto shaders_source = OpenGLShaderCompiler::SplitFileIntoShaders(source_path);
    const auto shader_ids = OpenGLShaderCompiler::CompileMultipleShaders(shaders_source);
    return Link(shader_ids);
}

GLuint OpenGLShaderCompiler::BuildFromCode(const std::string& code)
{
    const auto shaders_source = OpenGLShaderCompiler::SplitCodeIntoShaders(code);
    const auto shader_ids = OpenGLShaderCompiler::CompileMultipleShaders(shaders_source);
    return Link(shader_ids);
}

GLenum OpenGLShaderCompiler::GetShaderGLType(ShaderType type)
{
    GLenum GLtype = 0;

    switch (type)
    {
    case ShaderType::Vertex:
        GLtype = GL_VERTEX_SHADER;
        break;
    case ShaderType::Fragment:
        GLtype = GL_FRAGMENT_SHADER;
        break;
    case ShaderType::Geometry:
        GLtype = GL_GEOMETRY_SHADER;
        break;
    case ShaderType::TessellationControl:
        GLtype = GL_TESS_CONTROL_SHADER;
        break;
    case ShaderType::TessellationEvaluation:
        GLtype = GL_TESS_EVALUATION_SHADER;
        break;
    case ShaderType::Compute:
        GLtype = GL_COMPUTE_SHADER;
        break;
    default:
        Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Unknown Shader Type");
        break;
    }

    return GLtype;
}

std::string OpenGLShaderCompiler::GetShaderTypeString(ShaderType type)
{
    std::string type_str;

    switch (type)
    {
    case ShaderType::Vertex:
        type_str = "Vertex";
        break;
    case ShaderType::Fragment:
        type_str = "Fragment";
        break;
    case ShaderType::Geometry:
        type_str = "Geometry";
        break;
    case ShaderType::TessellationControl:
        type_str = "Tessellation Control";
        break;
    case ShaderType::TessellationEvaluation:
        type_str = "Tessellation Evaluation";
        break;
    case ShaderType::Compute:
        type_str = "Compute";
        break;
    default:
        Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Unknown Shader Type");
        break;
    }

    return type_str;
}

std::map<OpenGLShaderCompiler::ShaderType, std::string> OpenGLShaderCompiler::SplitCodeIntoShaders(
    const std::string& content)
{
    std::map<ShaderType, std::string> shader_code_map;

    std::string current_tag;
    ShaderType current_shader_type;
    std::string current_shader_code;
    std::istringstream content_stream(content);
    std::string line;

    auto SetShaderType = [&](const std::string& tag)
    {
        if (tag == "#vertex")
        {
            current_shader_type = ShaderType::Vertex;
        }
        else if (tag == "#fragment")
        {
            current_shader_type = ShaderType::Fragment;
        }
        else if (tag == "#geometry")
        {
            current_shader_type = ShaderType::Geometry;
        }
        else if (tag == "#tesscontrol")
        {
            current_shader_type = ShaderType::TessellationControl;
        }
        else if (tag == "#tesseval")
        {
            current_shader_type = ShaderType::TessellationEvaluation;
        }
        else if (tag == "#compute")
        {
            current_shader_type = ShaderType::Compute;
        }
    };

    while (std::getline(content_stream, line))
    {
        if (line.rfind("#vertex", 0) == 0 || line.rfind("#fragment", 0) == 0 ||
            line.rfind("#geometry", 0) == 0 || line.rfind("#tessellation", 0) == 0 ||
            line.rfind("#compute", 0) == 0)
        {
            if (!current_tag.empty())
            {
                shader_code_map[current_shader_type] = current_shader_code;
                current_shader_code.clear();
            }
            SetShaderType(line);
            current_tag = line;
        }
        else if (line.rfind("#include", 0) == 0)
        {
            std::istringstream include_stream(line);
            std::string directive, filepath;
            include_stream >> directive >> filepath;

            filepath = filepath.substr(1, filepath.length() - 2); // Remove quotes
            std::string included_content = ReadHeaderFile(PathConfig::GetShadersPath() / filepath);
            current_shader_code += included_content + "\n";
        }
        else
        {
            current_shader_code += line + "\n";
        }
    }

    if (!current_tag.empty())
    {
        shader_code_map[current_shader_type] = current_shader_code;
    }

    return shader_code_map;
}

std::map<OpenGLShaderCompiler::ShaderType, std::string> OpenGLShaderCompiler::SplitFileIntoShaders(
    const std::filesystem::path& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    return SplitCodeIntoShaders(content);
}

std::string OpenGLShaderCompiler::ReadHeaderFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Failed to open GLSL header file: " + path.string());

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
void OpenGLShaderCompiler::Pack(const std::filesystem::path& file_name, const Shader::ShaderBinaryData& data)
{
    try
    {
        std::ofstream bin_file(file_name, std::ios::out | std::ios::binary);
        bin_file.write(reinterpret_cast<const char*>(&data.format), sizeof(data.format));
        bin_file.write(reinterpret_cast<const char*>(&data.length), sizeof(data.length));
        bin_file.write(reinterpret_cast<const char*>(data.binary.data()), data.length);
    }
    catch (std::ifstream::failure& e)
    {
        Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Failed to pack shader: " + file_name.string());
        throw std::runtime_error("Failed to pack shader data");
    }
}

Shader::ShaderBinaryData OpenGLShaderCompiler::Unpack(const std::filesystem::path& file_name)
{
    Shader::ShaderBinaryData data;

    try
    {
        std::ifstream bin_file(file_name, std::ios::in | std::ios::binary);
        bin_file.read(reinterpret_cast<char*>(&data.format), sizeof(data.format));
        bin_file.read(reinterpret_cast<char*>(&data.length), sizeof(data.length));
        data.binary.resize(data.length);
        bin_file.read(reinterpret_cast<char*>(data.binary.data()), data.length);
    }
    catch (std::ifstream::failure& e)
    {
        Log::Write(Log::Levels::Critical, Log::Categories::Renderer, "Failed to unpack shader: " + file_name.string());
        throw std::runtime_error("Failed to unpack shader data");
    }

    return data;
}

void OpenGLShaderCompiler::SaveBinary(const std::filesystem::path& file_name, GLuint shader_id)
{
    Shader::ShaderBinaryData data;
    glGetProgramiv(shader_id, GL_PROGRAM_BINARY_LENGTH, &data.length);

    data.binary.resize(data.length);
    glGetProgramBinary(shader_id, data.length, &data.length, &data.format, data.binary.data());

    Pack(file_name, data);
}
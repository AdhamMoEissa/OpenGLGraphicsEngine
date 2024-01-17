#ifndef SHADER_H
#define SHADER_H

#include <Glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

struct ShaderSourceCode
{
public:
	std::string m_VertexShader;
	std::string m_GeometryShader;
	std::string m_FragmentShader;

	ShaderSourceCode()
	{

	}

	ShaderSourceCode(const char* vertexPath, const char* fragmentPath)
	{
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			//opens the files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;

			//reads the file contents into the string streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();

			//closes the file to avoid memory leaks
			vShaderFile.close();
			fShaderFile.close();

			//converts and saves the string stream into a string
			this->m_VertexShader = vShaderStream.str();
			this->m_GeometryShader = " ";
			this->m_FragmentShader = fShaderStream.str();
		}
		catch(std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
			std::cout << e.what() << std::endl;
		}
	}

	ShaderSourceCode(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
	{
		std::ifstream vShaderFile;
		std::ifstream gShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			//opens the files
			vShaderFile.open(vertexPath);
			gShaderFile.open(geometryPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, gShaderStream, fShaderStream;

			//reads the file contents into the string streams
			vShaderStream << vShaderFile.rdbuf();
			gShaderStream << gShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();

			//closes the file to avoid memory leaks
			vShaderFile.close();
			gShaderFile.close();
			fShaderFile.close();

			//converts and saves the string stream into a string
			this->m_VertexShader = vShaderStream.str();
			this->m_GeometryShader = gShaderStream.str();
			this->m_FragmentShader = fShaderStream.str();
		}
		catch(std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
			std::cout << e.what() << std::endl;
		}
	}

	ShaderSourceCode(ShaderSourceCode& sourceCode)
	{
		m_VertexShader = sourceCode.m_VertexShader;
		m_GeometryShader = sourceCode.m_GeometryShader;
		m_FragmentShader = sourceCode.m_FragmentShader;
	}

	ShaderSourceCode(ShaderSourceCode&& sourceCode)
	{
		m_VertexShader = sourceCode.m_VertexShader;
		m_GeometryShader = sourceCode.m_GeometryShader;
		m_FragmentShader = sourceCode.m_FragmentShader;
		
		sourceCode.m_VertexShader.erase();
		sourceCode.m_GeometryShader.erase();
		sourceCode.m_FragmentShader.erase();
	}

	~ShaderSourceCode()
	{

	}

	void implementUniform(unsigned int shaderIndex, const char* type, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformsPosition = shader.find("uniform ");
		std::string temp("uniform ");
		temp.reserve(50);
		temp += type;
		temp += ' ';
		temp += name;
		temp += ";\n";
		shader.insert(uniformsPosition, temp);
	}
	void implementTextureUniform(unsigned int shaderIndex, const char* type, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformsPosition = shader.find("uniform ");
		std::string temp("uniform ");
		temp.reserve(50);
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		shader.insert(uniformsPosition, temp);
	}

	void implementVertexLayout(unsigned int layoutIndex, const char* type, const char* name)
	{
		std::size_t layoutsPosition = m_VertexShader.find("layout");
		std::string temp("layout(location = ");
		temp.reserve(50);
		temp + std::to_string(layoutIndex);
		temp += ") in ";
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_VertexShader.insert(layoutsPosition, temp);
	}
	void implementGeometryLayout(char* inputPrimitive, char* outputPrimitive, unsigned int maxVertices)
	{
		if(*m_GeometryShader.c_str() == ' ') //if there is no geometry shader then no calculations should be made
			return;

		std::size_t layoutsStartPosition = m_GeometryShader.find("layout");
		std::size_t layoutsEndPosition = m_GeometryShader.find(";", m_GeometryShader.find("layout", m_GeometryShader.find("layout") + 6));
		m_GeometryShader.erase(layoutsStartPosition, layoutsEndPosition);
		
		std::string temp("layout(");
		temp.reserve(80);
		temp += inputPrimitive;
		temp += ") in;\n";
		temp += "layout(";
		temp += outputPrimitive;
		temp += ", max_vertices = ";
		temp += std::to_string(maxVertices);
		temp += ") out;\n";
		
		m_GeometryShader.insert(layoutsStartPosition, temp);
	}
	void implementFragmentLayout(unsigned int layoutIndex, const char* type, const char* name)
	{
		std::size_t layoutsPosition = m_FragmentShader.find("layout");
		std::string temp("layout(location = ");
		temp.reserve(50);
		temp += std::to_string(layoutIndex);
		temp += ") out ";
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_FragmentShader.insert(layoutsPosition, temp);
	}

	void implementGeometryInput(const char* type, const char* name)
	{
		std::size_t inputsPosition = m_GeometryShader.find("in ");
		std::string temp("in ");
		temp.reserve(30);
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_GeometryShader.insert(inputsPosition, temp);
	}
	void implementFragmentInput(const char* type, const char* name)
	{
		std::size_t inputsPosition = m_FragmentShader.find("in ");
		std::string temp("in ");
		temp.reserve(30);
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_FragmentShader.insert(inputsPosition, temp);
	}
	
	void implementVertexOutput(const char* type, const char* name)
	{
		std::size_t outputsPosition = m_VertexShader.find("out ");
		std::string temp("out ");
		temp.reserve(30);
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_VertexShader.insert(outputsPosition, temp);
	}
	void implementGeometryOutput(const char* type, const char* name)
	{
		std::size_t outputsPosition = m_GeometryShader.find("out ");
		std::string temp("out ");
		temp.reserve(30);
		temp += type;
		temp += " ";
		temp += name;
		temp += ";\n";
		m_GeometryShader.insert(outputsPosition, temp);
	}



	void unimplementUniform(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("uniform ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}
	void unimplementTexture(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("uniform ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}

	void unimplementVertexLayout(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("layout", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}
	void unimplementFragmentLayout(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("layout", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}

	void unimplementGeometryInput(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("in ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}
	void unimplementFragmentInput(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("in ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}

	void unimplementVertexOutput(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("out ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}
	void unimplementGeometryOutput(unsigned int shaderIndex, const char* name)
	{
		std::string& shader = this->operator[](shaderIndex);
		std::size_t uniformPosition = shader.rfind("out ", 0, shader.find(name));
		shader.erase(uniformPosition, shader.find(';\n', uniformPosition));
	}



	void completeAllImplementations()
	{
		while(m_VertexShader.find('\0') != std::string::npos)
		{
			m_VertexShader.erase(m_VertexShader.find('\0'));
		}
		while(m_GeometryShader.find('\0') != std::string::npos)
		{
			m_GeometryShader.erase(m_GeometryShader.find('\0'));
		}
		while(m_FragmentShader.find('\0') != std::string::npos)
		{
			m_FragmentShader.erase(m_FragmentShader.find('\0'));
		}

		m_VertexShader.reserve();
		m_GeometryShader.reserve();
		m_FragmentShader.reserve();
	}

	void operator=(ShaderSourceCode&& sourceCode)
	{
		m_VertexShader = sourceCode.m_VertexShader;
		m_GeometryShader = sourceCode.m_GeometryShader;
		m_FragmentShader = sourceCode.m_FragmentShader;

		sourceCode.m_VertexShader.erase();
		sourceCode.m_GeometryShader.erase();
		sourceCode.m_FragmentShader.erase();
	}
	void operator=(ShaderSourceCode& sourceCode)
	{
		m_VertexShader = sourceCode.m_VertexShader;
		m_GeometryShader = sourceCode.m_GeometryShader;
		m_FragmentShader = sourceCode.m_FragmentShader;
	}

	std::string& operator[](unsigned int index)
	{
		if(index == 0)
			return m_VertexShader;
		else if(index == 1)
			return m_GeometryShader;
		else if(index == 2)
			return m_FragmentShader;
	}
};

class Shader
{
public:
	//the shader program's m_ID number
	unsigned int m_ID;

	Shader()
		:m_ID(0)
	{
		
	}

	//Constructor read the shader file and builds the program
	//takes in a file path to the vertex and fragment shader source code (respectively)
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		m_SourceCode = new ShaderSourceCode(vertexPath, fragmentPath);

		const char* vShaderCode = m_SourceCode->m_VertexShader.c_str();
		const char* fShaderCode = m_SourceCode->m_FragmentShader.c_str();

		unsigned int vertex, fragment;

		//creates and compiles the vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX", vertexPath);
		//creates and compiles the fragment shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT", fragmentPath);
		//creates and links the shader program
		m_ID = glCreateProgram();
		glAttachShader(m_ID, vertex);
		glAttachShader(m_ID, fragment);
		glLinkProgram(m_ID);
		checkCompileErrors(m_ID, "PROGRAM", "program");

		//deletes the shader objects since they won't be used again
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	//takes in a file path to the vertex, geometry and fragment shader source code (respectively)
	Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
	{
		m_SourceCode = new ShaderSourceCode(vertexPath, geometryPath, fragmentPath);

		const char* vShaderCode = m_SourceCode->m_VertexShader.c_str();
		const char* gShaderCode = m_SourceCode->m_GeometryShader.c_str();
		const char* fShaderCode = m_SourceCode->m_FragmentShader.c_str();

		unsigned int vertex, geometry, fragment;

		//creates and compiles the vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX", vertexPath);
		//creates and compiles the geometry shader
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);
		checkCompileErrors(geometry, "GEOMETRY", geometryPath);
		//creates and compiles the fragment shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT", fragmentPath);
		//creates and links the shader program
		m_ID = glCreateProgram();
		glAttachShader(m_ID, vertex);
		glAttachShader(m_ID, geometry);
		glAttachShader(m_ID, fragment);
		glLinkProgram(m_ID);
		checkCompileErrors(m_ID, "PROGRAM", "Program");

		//deletes the shader objects since they won't be used again
		glDeleteShader(vertex);
		glDeleteShader(geometry);
		glDeleteShader(fragment);
	}

	Shader(ShaderSourceCode& sourceCode)
	{
		m_SourceCode = new ShaderSourceCode(sourceCode);
		if(sourceCode.m_GeometryShader.size() == 0)
		{
			const char* vShaderCode = sourceCode.m_VertexShader.c_str();
			const char* fShaderCode = sourceCode.m_FragmentShader.c_str();

			unsigned int vertexm_ID, fragmentm_ID;

			//creates and compiles the vertex shader
			vertexm_ID = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertexm_ID, 1, &vShaderCode, NULL);
			glCompileShader(vertexm_ID);
			checkCompileErrors(vertexm_ID, "VERTEX", "ShaderSourceCode");
			//creates and compiles the fragment shader
			fragmentm_ID = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragmentm_ID, 1, &fShaderCode, NULL);
			glCompileShader(fragmentm_ID);
			checkCompileErrors(fragmentm_ID, "FRAGMENT", "ShaderSourceCode");
			//creates and links the shader program
			m_ID = glCreateProgram();
			glAttachShader(m_ID, vertexm_ID);
			glAttachShader(m_ID, fragmentm_ID);
			glLinkProgram(m_ID);
			checkCompileErrors(m_ID, "PROGRAM", "ShaderSourceCode");
		}
		else
		{
			const char* vShaderCode = sourceCode.m_VertexShader.c_str();
			const char* gShaderCode = sourceCode.m_GeometryShader.c_str();
			const char* fShaderCode = sourceCode.m_FragmentShader.c_str();

			unsigned int vertex, geometry, fragment;

			//creates and compiles the vertex shader
			vertex = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertex, 1, &vShaderCode, NULL);
			glCompileShader(vertex);
			checkCompileErrors(vertex, "VERTEX", "ShaderSourceCode");
			//creates and compiles the geometry shader
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &gShaderCode, NULL);
			glCompileShader(geometry);
			checkCompileErrors(geometry, "GEOMETRY", "ShaderSourceCode");
			//creates and compiles the fragment shader
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment, 1, &fShaderCode, NULL);
			glCompileShader(fragment);
			checkCompileErrors(fragment, "FRAGMENT", "ShaderSourceCode");
			//creates and links the shader program
			m_ID = glCreateProgram();
			glAttachShader(m_ID, vertex);
			glAttachShader(m_ID, geometry);
			glAttachShader(m_ID, fragment);
			glLinkProgram(m_ID);
			checkCompileErrors(m_ID, "PROGRAM", "program");

			//deletes the shader objects since they won't be used again
			glDeleteShader(vertex);
			glDeleteShader(geometry);
			glDeleteShader(fragment);
		}
	}
	~Shader()
	{
		delete m_SourceCode;
	}

	//uses/activates the shader program
	void use()
	{
		glUseProgram(m_ID);
	}
	void unbind()
	{
		glUseProgram(0);
	}
	//destroys the current shader program
	void destroy()
	{
		glUseProgram(0);
		glDeleteProgram(m_ID);
	}
	//return ShaderSourceCode object
	ShaderSourceCode* getSourceCode()
	{
		return (ShaderSourceCode*)nullptr;
	}

	//utility uniform functions
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
	}
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
	}
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
	}
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
	}
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
	}
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w) const
	{
		glUniform4f(glGetUniformLocation(m_ID, name.c_str()), x, y, z, w);
	}
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
protected:
	//source code for each shader
	ShaderSourceCode* m_SourceCode;
	void checkCompileErrors(unsigned int shader, std::string type, std::string path)
	{
		int success;
		char infoLog[1024];
		if(type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if(!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << path << std::endl;
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetShaderiv(shader, GL_LINK_STATUS, &success);
			if(!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << path << std::endl;
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};

class DShader : public Shader
{
public:
	//Constructor read the shader file and builds the program
	//takes in a file path to the vertex and fragment shader source code (respectively)
	DShader(const char* vertexPath, const char* fragmentPath)
	{
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;

		m_SourceCode = new ShaderSourceCode(vertexPath, fragmentPath);
	}

	//takes in a file path to the vertex, geometry and fragment shader source code (respectively)
	DShader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
	{
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;

		m_SourceCode = new ShaderSourceCode(vertexPath, geometryPath, fragmentPath);
	}

	DShader(ShaderSourceCode* sourceCode)
	{
		m_SourceCode = new ShaderSourceCode(*sourceCode);

		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;
	}

	DShader(Shader& shader)
	{
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;

		m_SourceCode = new ShaderSourceCode(*shader.getSourceCode());
	}

	DShader(DShader&& shader)
	{
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;

		m_SourceCode = new ShaderSourceCode();
		*m_SourceCode = std::move(*shader.getSourceCode());

		shader.destroy();
	}
	DShader(Shader&& shader)
	{
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;

		m_SourceCode = new ShaderSourceCode();
		*m_SourceCode = std::move(*shader.getSourceCode());

		shader.destroy();
	}

	void destroy()
	{
		glUseProgram(0);
		glDeleteShader(m_VertexID);
		glDeleteShader(m_GeometryID);
		glDeleteShader(m_FragmentID);
		glDeleteProgram(m_ID);
	}
	//edits the source code of the shader of choice:
	//0 = vertex shader
	//1 = geometry shader
	//2 = fragment shader
	void edit(unsigned int index, std::string sourceCode)
	{
		m_SourceCode->operator[](index) = sourceCode;
		m_ShaderToBeCompiled[index] = 1;
	}
	void edit(ShaderSourceCode sourceCode)
	{
		*m_SourceCode = sourceCode;
		m_ShaderToBeCompiled[0] = 1;
		m_ShaderToBeCompiled[1] = 1;
		m_ShaderToBeCompiled[2] = 1;
	}
	//compiles (and links) the shader program to be ready to run
	void compile()
	{
		if(m_ShaderToBeCompiled[0])
		{
			//creates and compiles the vertex shader
			const char* source = m_SourceCode->m_VertexShader.c_str();
			glShaderSource(m_VertexID, 1, &source, NULL);
			glCompileShader(m_VertexID);
			checkCompileErrors(m_VertexID, "VERTEX", "inProgram");
		}
		if(m_ShaderToBeCompiled[1])
		{
			//creates and compiles the vertex shader
			const char* source = m_SourceCode->m_GeometryShader.c_str();
			glShaderSource(m_GeometryID, 1, &source, NULL);
			glCompileShader(m_GeometryID);
			checkCompileErrors(m_GeometryID, "GEOMETRY", "inProgram");
		}
		if(m_ShaderToBeCompiled[2])
		{
			//creates and compiles the fragment shader
			const char* source = m_SourceCode->m_FragmentShader.c_str();
			glShaderSource(m_FragmentID, 1, &source, NULL);
			glCompileShader(m_FragmentID);
			checkCompileErrors(m_FragmentID, "FRAGMENT", "inProgram");
		}
		//creates and links the shader program
		glAttachShader(m_ID, m_VertexID);
		glAttachShader(m_ID, m_GeometryID);
		glAttachShader(m_ID, m_FragmentID);
		glLinkProgram(m_ID);
		checkCompileErrors(m_ID, "PROGRAM", "inProgram");


		m_ShaderToBeCompiled[0] = 0;
		m_ShaderToBeCompiled[1] = 0;
		m_ShaderToBeCompiled[2] = 0;
	}
	//copies the source code of the right hand operand to this current shader object
	void operator<<(DShader& d)
	{
		this->copy(d);
	}
	void operator<<(Shader& s)
	{
		this->copy(s);
	}
private:
	//internal ID for each shader
	unsigned int m_VertexID, m_GeometryID, m_FragmentID;
	//tells the compile method which shader needs to be compiled and which doesn't
	bool m_ShaderToBeCompiled[3];

	void copy(DShader& d)
	{
		this->m_SourceCode->m_VertexShader = d.m_SourceCode->m_VertexShader;
		this->m_SourceCode->m_GeometryShader = d.m_SourceCode->m_GeometryShader;
		this->m_SourceCode->m_FragmentShader = d.m_SourceCode->m_FragmentShader;
	}
	void copy(Shader& s)
	{
		DShader temp(s);
		this->m_SourceCode->m_VertexShader = temp.m_SourceCode->m_VertexShader;
		this->m_SourceCode->m_GeometryShader = temp.m_SourceCode->m_GeometryShader;
		this->m_SourceCode->m_FragmentShader = temp.m_SourceCode->m_FragmentShader;
	}
};

#endif
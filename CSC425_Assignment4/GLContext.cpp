#include "GLContext.h"
#include "LoadShaders.h"
#include "wglext.h"
#include <iostream>
#include <math.h>
#include <Windows.h>
#include <locale>
#include <codecvt>
#include <string>
#include <glm.hpp>
#include <mat4x4.hpp>
#include <vec3.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace GLContext {

	bool validAttribLocation(GLuint location);

	GLContext::GLContext()
	{
		this->success = false;
		this->shaderPrograms = std::unordered_map<const char*, GLuint>();
	}


	GLContext::~GLContext()
	{


		if (this->numVAOs > 0)
		{
			glDeleteVertexArrays(this->numVAOs, this->VAOs);
			glDeleteBuffers(this->numVBOs, this->VBOs);
			delete this->VAOs;
			delete this->VBOs;
			delete this->models;
		}
	}


	bool GLContext::initContext(int argc, char** argv, displayFunc dFunc, reshapeFunc rFunc)
	{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
		glutInitWindowSize(768, 768);
		glutInitContextVersion(4, 3);
		glutInitContextProfile(GLUT_FORWARD_COMPATIBLE);
		glutCreateWindow(argv[0]);
		
		glEnable(GL_DEPTH_TEST);

		glutIdleFunc(dFunc);
		glutReshapeFunc(rFunc);

		if (glewInit()) {
			std::cerr << "Unable to initialize GLEW ... exiting" << std::endl;
			this->success = false;
			return false;
		}

		vec3 cameraPosition;
		cameraPosition.x = 0;
		cameraPosition.y = 0;
		cameraPosition.z = -3;
		vec3 cameraGaze = { 0.0, 0.0, 0.0 };

		initCamera(cameraPosition, cameraGaze);

		this->success = true;
		return true;
	}

	bool GLContext::initShaders(const material s[], int n)
	{
		if (this->success)
		{
			for (int i = 0; i < n; i++)
			{
				ShaderInfo shaders[] = {
						{ GL_VERTEX_SHADER, s[i].vertexShader },
						{ GL_FRAGMENT_SHADER, s[i].fragmentShader },
						{ GL_NONE, NULL }
				};

				GLuint program = LoadShaders(shaders);

				std::pair<const char*, GLuint> shaderProgram(s[i].materialName, program);

				this->shaderPrograms.insert(shaderProgram);
			}

			return true;
		}
		return false;
	}

	bool GLContext::initModels(const model m[], int n)
	{
		if (this->success)
		{


			this->numVAOs = n;
			this->numVBOs = n;
			this->VAOs = new GLuint[n];
			this->VBOs = new GLuint[n];
			this->models = new model[n];

			glGenVertexArrays(this->numVAOs, this->VAOs);

			glGenBuffers(n, this->VBOs);

			// Gen texture

			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			// Load image

			float pixels[] = {
				.59, .42, .29,
				.47, .34, .23,
				.73, .52, .36,
				.59, .42, .29,
				.59, .42, .29,
				.47, .34, .23,
				.47, .34, .23,
				.73, .52, .36,
				.47, .34, .23,
				.59, .42, .29,
				.47, .34, .23,
				.47, .34, .23,
				.35, .24, .16,
				.35, .24, .16,
				.47, .34, .23,
				.59, .42, .29
			};
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_FLOAT, pixels);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glGenerateMipmap(GL_TEXTURE_2D);

			GLfloat *texCoords = new GLfloat[72];

			for (int i = 0; i < 6; i++)
			{
				texCoords[i * 12] = 1;
				texCoords[i * 12 + 1] = 0;
				texCoords[i * 12 + 2] = 1;
				texCoords[i * 12 + 3] = 1;
				texCoords[i * 12 + 4] = 0;
				texCoords[i * 12 + 5] = 1;
				texCoords[i * 12 + 6] = 1;
				texCoords[i * 12 + 7] = 0;
				texCoords[i * 12 + 8] = 0;
				texCoords[i * 12 + 9] = 1;
				texCoords[i * 12 + 10] = 0;
				texCoords[i * 12 + 11] = 0;
			}

			// End load texture


			for (int i = 0; i < n; i++)
			{
				int vertexCount = m[i].vertices.size();
				int normalCount = m[i].normals.size();

				glBindVertexArray(this->VAOs[i]);
				glBindBuffer(GL_ARRAY_BUFFER, this->VBOs[i]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (vertexCount + normalCount + 72), NULL, GL_STATIC_DRAW);

				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * vertexCount, &m[i].vertices.front());
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexCount, sizeof(GLfloat) * normalCount, &m[i].normals.front());
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (vertexCount + normalCount), sizeof(GLfloat) * 72, texCoords);

				GLuint program = this->shaderPrograms[m[i].shader];
				glUseProgram(program);

				GLuint vPosition = glGetAttribLocation(program, "vPosition");
				glEnableVertexAttribArray(vPosition);
				glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

				GLuint vNormal = glGetAttribLocation(program, "vNormal");

				if (validAttribLocation(vNormal))
				{
					glEnableVertexAttribArray(vNormal);
					glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(GLfloat) * vertexCount));
				}

				GLuint vTexCoord = glGetAttribLocation(program, "texcoord");

				glEnableVertexAttribArray(vTexCoord);
				glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(GLfloat) * (vertexCount + normalCount)));
				

				this->models[i] = m[i];
			}

			return true;
		}
		return false;
	}

	void GLContext::initCamera(vec3 position, vec3 gaze)
	{
		if (!this->cameraInitialized)
		{
			delete camera;
		}
		this->camera = new Camera(glm::vec3(position.x, position.y, position.z), glm::vec3(gaze.x, gaze.y, gaze.z));
		this->cameraInitialized = true;

	}

	void GLContext::initLight(float x, float y, float z)
	{
		this->light = { x, y, z };
	}

	void GLContext::run()
	{
		if (this->success)
		{
			glm::mat4 proj = glm::perspective<float>(45.0f, this->w / this->h, 0.01f, 100.0f);
			glm::mat4 view = glm::mat4() * glm::lookAt(this->camera->getPosition(), this->camera->getGaze(), glm::vec3(0.0, 1.0, 0.0));

			for (int i = 0; i < this->numVAOs; i++)
			{
				model m = this->models[i];

				GLuint program = this->shaderPrograms[m.shader];
				glUseProgram(program);
				glBindVertexArray(this->VAOs[i]);

				GLint projLoc = glGetUniformLocation(program, "projection");
				GLint viewLoc = glGetUniformLocation(program, "view");

				glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
				glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
			}



			glutMainLoop();
		}
	}

	void GLContext::render()
	{
		if (this->success)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0, 0.0, 0.0, 1.0);

			int deltaTime = glutGet(GLUT_ELAPSED_TIME);

			for (int i = 0; i < this->numVAOs; i++)
			{
				model m = this->models[i];

				GLuint program = this->shaderPrograms[m.shader];
				glUseProgram(program);
				glBindVertexArray(this->VAOs[i]);

				float theta = fmod(m.rotationTheta * deltaTime, 360);

				glm::mat4 mod = glm::mat4(1.0f);
				glm::vec3 rotationTheta = glm::vec3(m.rotationAxis == DIRECTION::LEFT, m.rotationAxis == DIRECTION::UP, m.rotationAxis == DIRECTION::FRONT);

				mod = glm::translate(mod, glm::vec3(m.position.x, m.position.y, m.position.z));
				mod = glm::rotate(mod, theta, rotationTheta);

				GLint modelLoc = glGetUniformLocation(program, "model");

				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mod));

				glDrawArrays(this->models[i].renderType, 0, this->models[i].vertices.size() / VECTOR_SIZE);
			}

			glFlush();
			glutSwapBuffers();
		}
	}

	void GLContext::reshape(int w, int h)
	{
		this->w = (float)w;
		this->h = (float)h;
	}

	bool validAttribLocation(GLuint location)
	{
		int max_attribs;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);

		return location < (GLuint)max_attribs;
	}
}
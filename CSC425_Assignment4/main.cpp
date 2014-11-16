#include <iostream>
#include "GLContext.h"
#include "Cube.h"

using namespace std;

void display(void);
void reshape(int w, int h);

GLContext::GLContext *glContext;

int main(int argc, char *argv[])
{
	glContext = new GLContext::GLContext();

	GLContext::material materials[] = {
			{ "basic", "basic.vert", "basic.frag" },
			{ "specular", "shiny.vert", "basic.frag" }
	};

	vector<GLContext::model> models = vector<GLContext::model>();

	models.push_back(Cube({ 1.5, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, COLOR::RED, "specular", 50.0).modelData()); // broad specular
	models.push_back(Cube({ -1.5, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, COLOR::GREEN, "basic", 0.0).modelData()); // matte
	models.push_back(Cube({ 0.0, 1.5, 0.0 }, { 1.0, 1.0, 1.0 }, COLOR::BLUE, "specular", 10.0).modelData()); // hybrid
	
	if (glContext->initContext(argc, argv, display, reshape) &&
		glContext->initShaders(materials, 2) &&
		glContext->initModels(&models.front(), models.size()));
	{
		glContext->initLight(0, 0, -0.5);
		glContext->run();
	}

	delete glContext;

	return EXIT_SUCCESS;
}

void display(void)
{
	glContext->render();
}

void reshape(int w, int h)
{
	glContext->reshape(w, h);
}
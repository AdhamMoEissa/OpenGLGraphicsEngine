#ifndef CAMERA_H
#define CAMERA_H

#include <Glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum CameraMovement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 4.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 75.0f;

class Camera
{
public:
	//Camera Attributes/Properties
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	//euler angles (Hopefully will be switched to a quaternion)
	float Yaw;
	float Pitch;
	//Camera Options/Settings
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	//constructor to initialize all values of the camera object
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
		: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	//Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
		: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	void ProcessKeyboard(CameraMovement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if(direction == FORWARD)
			Position += Front * velocity;
		if(direction == BACKWARD)
			Position -= Front * velocity;
		if(direction == LEFT)
			Position -= Right * velocity;
		if(direction == RIGHT)
			Position += Right * velocity;
		if(direction == UP)
			Position += WorldUp * velocity;
		if(direction == DOWN)
			Position -= WorldUp * velocity;

	}

	void ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch = true)
	{
		xOffset *= MouseSensitivity * (0.02f * Zoom + 0.1f);
		yOffset *= MouseSensitivity * (0.02f * Zoom + 0.1f);

		Yaw += xOffset;
		Pitch += yOffset;

		if(constrainPitch)
		{
			if(Pitch > 89.0f)
				Pitch = 89.0f;
			if(Pitch < -89.0f)
				Pitch = -89.0f;
		}

		updateCameraVectors();
	}

	void ProcessMouseScroll(float yOffset)
	{
		Zoom -= (float)yOffset;
		if(Zoom < 1.0f)
			Zoom = 1.0f;
		if(Zoom > 90.0f)
			Zoom = 90.0f;
	}

private:
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);

		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};

#endif

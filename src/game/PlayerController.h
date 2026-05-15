#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PlayerController {
public:
    PlayerController() : m_Position(0.0f), m_Yaw(0.0f), m_FirstMouse(true), m_LastX(0.0), m_LastY(0.0) {}

    void init(GLFWwindow* window) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void update(GLFWwindow* window, float deltaTime) {
        const float SPEED = 5.0f;
        
        // Horizontal rotation math
        glm::vec3 forward = glm::vec3(sin(glm::radians(m_Yaw)), 0.0f, cos(glm::radians(m_Yaw)));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            m_Position -= forward * SPEED * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            m_Position += forward * SPEED * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            m_Position -= right * SPEED * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            m_Position += right * SPEED * deltaTime;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (m_FirstMouse) {
            m_LastX = xpos;
            m_LastY = ypos;
            m_FirstMouse = false;
        }

        float xoffset = static_cast<float>(xpos - m_LastX);
        m_LastX = xpos;
        m_LastY = ypos;

        const float sensitivity = 0.1f;
        xoffset *= sensitivity;

        m_Yaw -= xoffset; // Minus because moving mouse right (positive xoffset) should turn character right (negative yaw in OpenGL space)
    }

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Position);
        model = glm::rotate(model, glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        return model;
    }

    glm::vec3 getCameraPosition() const {
        return m_Position + glm::vec3(0.0f, 1.7f, 0.0f);
    }

    glm::mat4 getViewMatrix() const {
        glm::vec3 cameraPos = m_Position - glm::vec3(sin(glm::radians(m_Yaw)), -0.5f, cos(glm::radians(m_Yaw))) * 4.0f;
        cameraPos.y += 2.0f;
        glm::vec3 targetPos = m_Position + glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(cameraPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    }

private:
    glm::vec3 m_Position;
    float m_Yaw;
    bool m_FirstMouse;
    double m_LastX;
    double m_LastY;
};

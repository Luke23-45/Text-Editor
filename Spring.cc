#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
template<typename T>
T clamp(T value, T min, T max) {
    return std::max(min, std::min(value, max));
}
class AdvancedSpringPhysics {
public:
    struct SpringState {
        double anchorX;         // Fixed anchor point X
        double anchorY;         // Fixed anchor point Y
        double currentX;        // Current mass X position
        double currentY;        // Current mass Y position
        double restLength;      // Natural length of spring
        double currentLength;   // Current length
        double stiffness;       // Spring constant
        double damping;         // Damping coefficient
        double mass;            // Mass of the hanging object
        double velocityX;       // Velocity in X direction
        double velocityY;       // Velocity in Y direction
        bool isDragged;         // Whether spring is being dragged
        double minLength;       // Minimum compressed length (solid length)
        double maxLength;       // Maximum extended length
        bool isLimitReached;    // Flag to track if limit is reached during dragging
        double lastDragX;       // Last dragged X position
        double lastDragY;       // Last dragged Y position
        double initialReleaseVelocityX;  // Initial velocity on release
        double initialReleaseVelocityY;  // Initial velocity on release
    };

    SpringState state;

    AdvancedSpringPhysics() {
        // Initialize spring state with length constraints
        state = {
            SCREEN_WIDTH / 2.0,  // anchorX
            100.0,               // anchorY
            SCREEN_WIDTH / 2.0,  // currentX
            300.0,               // currentY
            200.0,               // restLength
            200.0,               // currentLength
            100.0,               // stiffness
            0.3,                 // damping
            1.0,                 // mass
            0.0,                 // velocityX
            0.0,                 // velocityY
            false,               // not dragged initially
            100.0,               // minLength (minimum compression)
            450.0,               // maxLength (maximum extension)
            false,               // limit not reached initially
            SCREEN_WIDTH / 2.0,  // last drag X
            300.0,               // last drag Y
            0.0,                 // initial release velocity X
            0.0                  // initial release velocity Y
        };
    }

    void update(double deltaTime) {
        // If dragged, calculate potential velocities
        if (state.isDragged) {
            // Calculate velocity based on last drag position
            double dx = state.currentX - state.lastDragX;
            double dy = state.currentY - state.lastDragY;
            
            // Store initial release velocities
            state.initialReleaseVelocityX = dx / deltaTime;
            state.initialReleaseVelocityY = dy / deltaTime;

            // Update last drag position
            state.lastDragX = state.currentX;
            state.lastDragY = state.currentY;

            return;  // Skip physics update while dragging
        }

        // Standard physics update
        // Calculate displacement from rest position
        double dx = state.currentX - state.anchorX;
        double dy = state.currentY - state.anchorY;
        double currentLength = std::sqrt(dx*dx + dy*dy);

        // Initial velocities after release
        if (std::abs(state.initialReleaseVelocityX) > 0 || 
            std::abs(state.initialReleaseVelocityY) > 0) {
            state.velocityX = state.initialReleaseVelocityX;
            state.velocityY = state.initialReleaseVelocityY;
            
            // Reset initial release velocities
            state.initialReleaseVelocityX = 0;
            state.initialReleaseVelocityY = 0;
        }

        // Spring force calculation
        double springForceX = -state.stiffness * (currentLength - state.restLength) * (dx / currentLength);
        double springForceY = -state.stiffness * (currentLength - state.restLength) * (dy / currentLength);

        // Damping force
        double dampingForceX = -state.damping * state.velocityX;
        double dampingForceY = -state.damping * state.velocityY;

        // Gravitational force
        double gravityForceY = state.mass * 9.8;

        // Update velocities
        state.velocityX += (springForceX + dampingForceX) / state.mass * deltaTime;
        state.velocityY += (springForceY + dampingForceY + gravityForceY) / state.mass * deltaTime;

        // Update position
        state.currentX += state.velocityX * deltaTime;
        state.currentY += state.velocityY * deltaTime;

        // Enforce length constraints
        dx = state.currentX - state.anchorX;
        dy = state.currentY - state.anchorY;
        currentLength = std::sqrt(dx*dx + dy*dy);

        if (currentLength < state.minLength) {
            double angle = std::atan2(dy, dx);
            state.currentX = state.anchorX + state.minLength * std::cos(angle);
            state.currentY = state.anchorY + state.minLength * std::sin(angle);
            
            // Reverse velocity with some energy loss
            state.velocityX *= -0.5;
            state.velocityY *= -0.5;
        } else if (currentLength > state.maxLength) {
            double angle = std::atan2(dy, dx);
            state.currentX = state.anchorX + state.maxLength * std::cos(angle);
            state.currentY = state.anchorY + state.maxLength * std::sin(angle);
            
            // Reverse velocity with some energy loss
            state.velocityX *= -0.5;
            state.velocityY *= -0.5;
        }
    }

    void dragTo(double mouseX, double mouseY) {
        // Calculate potential new length
        double dx = mouseX - state.anchorX;
        double dy = mouseY - state.anchorY;
        double potentialLength = std::sqrt(dx*dx + dy*dy);
        double angle = std::atan2(dy, dx);

        // Check if dragging is within limits
        if (potentialLength >= state.minLength && potentialLength <= state.maxLength) {
            state.isDragged = true;
            state.currentX = mouseX;
            state.currentY = mouseY;
            state.lastDragX = mouseX;
            state.lastDragY = mouseY;
            state.velocityX = 0;
            state.velocityY = 0;
            state.isLimitReached = false;
        } 
        // If outside limits, check if limit is already reached
        else {
            state.isDragged = true;
            
            // If not already at a limit, set the position to the limit
            if (!state.isLimitReached) {
                if (potentialLength < state.minLength) {
                    state.currentX = state.anchorX + state.minLength * std::cos(angle);
                    state.currentY = state.anchorY + state.minLength * std::sin(angle);
                    state.lastDragX = state.currentX;
                    state.lastDragY = state.currentY;
                    state.isLimitReached = true;
                } else if (potentialLength > state.maxLength) {
                    state.currentX = state.anchorX + state.maxLength * std::cos(angle);
                    state.currentY = state.anchorY + state.maxLength * std::sin(angle);
                    state.lastDragX = state.currentX;
                    state.lastDragY = state.currentY;
                    state.isLimitReached = true;
                }
            }
        }
    }

    void release() {
        // Reset dragging state
        state.isDragged = false;
        state.isLimitReached = false;
    }
};

class SpringRenderer {
private:
    SDL_Renderer* renderer;
    AdvancedSpringPhysics& physics;

    // Rendering configuration

    struct RenderConfig {
        int coils;
        double springRadius;
        double wireThickness;  // Now used as base thickness
        double maxWireThickness;  // New parameter for maximum thickness
        SDL_Color neutralColor;
        SDL_Color stretchColor;
        SDL_Color compressColor;
        double maxDeformationThreshold;
    };

    RenderConfig renderConfig;

    // Helper method to interpolate colors
    SDL_Color interpolateColor(const SDL_Color& color1, const SDL_Color& color2, double factor) {
        factor = std::max(0.0, std::min(1.0, factor));
        SDL_Color result;
        result.r = static_cast<Uint8>(color1.r * (1 - factor) + color2.r * factor);
        result.g = static_cast<Uint8>(color1.g * (1 - factor) + color2.g * factor);
        result.b = static_cast<Uint8>(color1.b * (1 - factor) + color2.b * factor);
        result.a = 255;
        return result;
    }
    void drawAntialiasedThickLine(SDL_Renderer* renderer, 
                                  float x0, float y0, 
                                  float x1, float y1, 
                                  float thickness,
                                  const SDL_Color& color) {
        // Ensure positive thickness
        thickness = std::max(1.0f, thickness);
        
        // Compute line vector
        float dx = x1 - x0;
        float dy = y1 - y0;
        float length = std::sqrt(dx*dx + dy*dy);
        
        // Normalize and rotate perpendicular vector
        float perpX = -dy / length;
        float perpY = dx / length;
        
        // Half-thickness offset
        float halfThickness = thickness * 0.5f;

        // Create vertices for a thick line
        std::vector<SDL_FPoint> lineVertices(4);
        lineVertices[0] = {x0 + perpX * halfThickness, y0 + perpY * halfThickness};
        lineVertices[1] = {x0 - perpX * halfThickness, y0 - perpY * halfThickness};
        lineVertices[2] = {x1 - perpX * halfThickness, y1 - perpY * halfThickness};
        lineVertices[3] = {x1 + perpX * halfThickness, y1 + perpY * halfThickness};

        // Anti-aliasing for edges
        auto drawAntialiasedEdge = [&](float x0, float y0, float x1, float y1) {
            float dx = x1 - x0;
            float dy = y1 - y0;
            float length = std::sqrt(dx*dx + dy*dy);
            
            for (float t = 0; t <= 1.0f; t += 1.0f / length) {
                float x = x0 + t * dx;
                float y = y0 + t * dy;
                
                // Compute alpha based on distance from line center
                float dist = std::abs(perpX * (x - x0) + perpY * (y - y0));
                Uint8 alpha = static_cast<Uint8>(
                    std::max(0.0f, std::min(1.0f, (halfThickness - dist) / halfThickness)) * 255
                );
                
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
                SDL_RenderDrawPointF(renderer, x, y);
            }
        };

        // Fill the line body
        for (float t = 0; t <= 1.0f; t += 1.0f / length) {
            float x = x0 + t * dx;
            float y = y0 + t * dy;
            
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            
            // Draw thick line points
            for (float offset = -halfThickness; offset <= halfThickness; offset += 1.0f) {
                SDL_RenderDrawPointF(renderer, 
                    x + perpX * offset, 
                    y + perpY * offset
                );
            }
        }

        // Anti-aliased edges
        drawAntialiasedEdge(lineVertices[0].x, lineVertices[0].y, lineVertices[1].x, lineVertices[1].y);
        drawAntialiasedEdge(lineVertices[2].x, lineVertices[2].y, lineVertices[3].x, lineVertices[3].y);
    }

    // Smooth gradient thick line drawing method
    void drawSmoothGradientThickLine(SDL_Renderer* renderer, 
                                     float x0, float y0, 
                                     float x1, float y1, 
                                     float thickness,
                                     const SDL_Color& startColor, 
                                     const SDL_Color& endColor) {
        float dx = x1 - x0;
        float dy = y1 - y0;
        float length = std::sqrt(dx*dx + dy*dy);

        for (float t = 0; t <= 1.0f; t += 1.0f / length) {
            // Interpolate position
            float x = x0 + t * dx;
            float y = y0 + t * dy;

            // Interpolate color
            SDL_Color currentColor = {
                static_cast<Uint8>(startColor.r * (1.0f - t) + endColor.r * t),
                static_cast<Uint8>(startColor.g * (1.0f - t) + endColor.g * t),
                static_cast<Uint8>(startColor.b * (1.0f - t) + endColor.b * t),
                static_cast<Uint8>(startColor.a * (1.0f - t) + endColor.a * t)
            };

            // Draw thick anti-aliased line
            drawAntialiasedThickLine(
                renderer, 
                x, y, 
                x + 1, y + 1,  // Tiny line segment
                thickness, 
                currentColor
            );
        }
    }


public:
      SpringRenderer(SDL_Renderer* renderer, AdvancedSpringPhysics& phys)
        : renderer(renderer), physics(phys) {
        
        renderConfig = {
            9,                      // number of coils
            25.0,                    // spring radius
            3.0,                     // base wire thickness
            20.0,                     // max wire thickness
            {200, 200, 200, 255},    // neutral color (soft blue-gray)
            {255, 50, 50, 255},      // stretch color (red)
            {50, 50, 105, 255},      // compress color (blue)
            0.2                 // deformation threshold for color change
        };
    }


    void render() {
        auto& state = physics.state;

        // Calculate spring parameters
        double anchorX = state.anchorX;
        double anchorY = state.anchorY;
        double currentX = state.currentX;
        double currentY = state.currentY;

        // Calculate spring direction and length
        double dx = currentX - anchorX;
        double dy = currentY - anchorY;
        double currentLength = std::sqrt(dx*dx + dy*dy);
        double angle = std::atan2(dy, dx);

        // Calculate deformation
    double deformationFactor;
    double absoluteDeformation;


    if (currentLength < state.restLength) {
        // Compression
        deformationFactor = (state.restLength - currentLength) / (state.restLength - state.minLength);
        absoluteDeformation = deformationFactor;
    } else {
        // Extension
        deformationFactor = (currentLength - state.restLength) / (state.maxLength - state.restLength);
        absoluteDeformation = deformationFactor;
    }

    SDL_Color springColor;
    if (std::abs(deformationFactor) < renderConfig.maxDeformationThreshold) {
        // Neutral state
        springColor = renderConfig.neutralColor;
    } else if (currentLength < state.restLength) {
        // Compressing (blue)
        springColor = interpolateColor(
            renderConfig.neutralColor, 
            renderConfig.compressColor, 
            std::min(1.0, std::abs(deformationFactor))
        );
    } else {
        // Stretching (red)
        springColor = interpolateColor(
            renderConfig.neutralColor, 
            renderConfig.stretchColor, 
            std::min(1.0, std::abs(deformationFactor))
        );
    }
      //  double absoluteDeformation = std::abs(deformationFactor);


        // Dynamic radius based on spring deformation
        double dynamicRadius = renderConfig.springRadius * (1.0 - absoluteDeformation * 0.3);

        
        // Limit the minimum radius to prevent extreme deformation
        dynamicRadius = std::max(dynamicRadius, renderConfig.springRadius * 0.3);

        // Set the dynamic color
        SDL_SetRenderDrawColor(renderer, springColor.r, springColor.g, springColor.b, springColor.a);
    
        std::vector<SDL_FPoint> springPoints;
        for (int i = 0; i <= renderConfig.coils * 20; ++i) {
            double t = static_cast<double>(i) / (renderConfig.coils * 20);
            
            // Parametric equations for helical spring with dynamic radius
            double coilAngle = t * renderConfig.coils * 2 * M_PI;
            double perpAngle = angle + M_PI/2;
            
            double x = anchorX + 
                       t * currentLength * std::cos(angle) + 
                       dynamicRadius * std::sin(coilAngle) * std::cos(perpAngle);
            
            double y = anchorY + 
                       t * currentLength * std::sin(angle) + 
                       dynamicRadius * std::sin(coilAngle) * std::sin(perpAngle);

            springPoints.push_back({static_cast<float>(x), static_cast<float>(y)});
        }
        double baseThickness = renderConfig.wireThickness;
        double minThickness = 1.0;  // Minimum wire thickness
        double maxThickness = renderConfig.wireThickness * 2.0;  // Maximum wire thickness

        // Thickness changes based on deformation
        double dynamicWireThickness;
        if (currentLength < state.restLength) {
            // Compression: increase thickness
            dynamicWireThickness = baseThickness * (1.0 + absoluteDeformation * 0.5);
        } else {
            // Extension: decrease thickness
            dynamicWireThickness = baseThickness * (1.0 - absoluteDeformation * 0.5);
        }

        // Clamp thickness
        dynamicWireThickness = clamp(
            dynamicWireThickness, 
            minThickness, 
            maxThickness
        );
        // Draw smooth gradient spring
        for (size_t i = 1; i < springPoints.size(); ++i) {
            // Create a gradient between neutral and deformed colors
            SDL_Color startColor = interpolateColor(
                renderConfig.neutralColor, 
                (currentLength < state.restLength) ? renderConfig.compressColor : renderConfig.stretchColor, 
                0.3f
            );
            
            SDL_Color endColor = interpolateColor(
                renderConfig.neutralColor, 
                (currentLength < state.restLength) ? renderConfig.compressColor : renderConfig.stretchColor, 
                0.7f
            );

            // Draw smooth gradient thick line
            drawSmoothGradientThickLine(
                renderer, 
                springPoints[i-1].x, springPoints[i-1].y, 
                springPoints[i].x, springPoints[i].y, 
                dynamicWireThickness, 
                startColor, 
                endColor
            );
        }

        // Add soft glow effect
        SDL_Color glowColor = springColor;
        glowColor.a = 30;  // Very soft glow
        for (size_t i = 1; i < springPoints.size(); ++i) {
            drawAntialiasedThickLine(
                renderer, 
                 springPoints[i-1].x, springPoints[i-1].y, 
                springPoints[i].x, springPoints[i].y, 
                dynamicWireThickness, 
                glowColor
            );
        }

        // Render anchor point
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_Rect anchorRect = {
            static_cast<int>(anchorX) - 5, 
            static_cast<int>(anchorY) - 5, 
            10, 10
        };
        SDL_RenderFillRect(renderer, &anchorRect);

        // Render mass/end point
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect massRect = {
            static_cast<int>(currentX) - 5, 
            static_cast<int>(currentY) - 5, 
            10, 10
        };
        SDL_RenderFillRect(renderer, &massRect);
    }
};

class SpringSimulation {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    AdvancedSpringPhysics physics;
    SpringRenderer* springRenderer;
    bool running;
    bool isDragging;

public:
    SpringSimulation() : window(nullptr), renderer(nullptr), running(true), isDragging(false) {
        if (initSDL()) {
            springRenderer = new SpringRenderer(renderer, physics);
        }
    }

    ~SpringSimulation() {
        cleanup();
    }

    bool initSDL() {
        SDL_Init(SDL_INIT_VIDEO);
        
        window = SDL_CreateWindow(
            "Interactive Spring Simulation", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            SCREEN_WIDTH, SCREEN_HEIGHT, 
            SDL_WINDOW_SHOWN
        );
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
         return true;
    }

    void run() {
        SDL_Event event;
        Uint64 lastTime = SDL_GetTicks64();

        while (running) {
            while (SDL_PollEvent(&event)) {
                handleEvent(event);
            }

            // Update physics
            Uint64 currentTime = SDL_GetTicks64();
            double deltaTime = (currentTime - lastTime) / 1000.0;
            lastTime = currentTime;

            physics.update(deltaTime);

            // Render
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black background
            SDL_RenderClear(renderer);

            springRenderer->render();  // Render spring

            SDL_RenderPresent(renderer);

            SDL_Delay(16);  // Limit FPS to ~60
        }
    }

    void handleEvent(SDL_Event& event) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                // Check if mouse is near the mass
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                int massX = static_cast<int>(physics.state.currentX);
                int massY = static_cast<int>(physics.state.currentY);
                if (std::hypot(mouseX - massX, mouseY - massY) < 10) {
                    physics.dragTo(mouseX, mouseY);  // Start dragging
                    isDragging = true;
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT && isDragging) {
                physics.release();  // Release the spring
                isDragging = false;
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            if (isDragging) {
                physics.dragTo(event.motion.x, event.motion.y);  // Update position while dragging
            }
        }
    }

    void cleanup() {
        delete springRenderer;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    SpringSimulation simulation;
    simulation.run();
    return 0;
}
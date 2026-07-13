#include <SFML/Graphics.hpp>
#include <cmath>
#include <ctime>

const int WINDOW_SIZE = 600;
const float PI = 3.14159265f;

int main() {
    // Create anti-aliased window for smooth hand lines
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "C++ Real-Time Analogue Clock", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    sf::Vector2f center(WINDOW_SIZE / 2.0f, WINDOW_SIZE / 2.0f);

    // 1. Clock Face Setup
    sf::CircleShape clockFace(250.f);
    clockFace.setOrigin(250.f, 250.f);
    clockFace.setPosition(center);
    clockFace.setFillColor(sf::Color(20, 20, 20));
    clockFace.setOutlineThickness(8.f);
    clockFace.setOutlineColor(sf::Color(200, 200, 200));

    // Center Pin Setup
    sf::CircleShape centerPin(8.f);
    centerPin.setOrigin(8.f, 8.f);
    centerPin.setPosition(center);
    centerPin.setFillColor(sf::Color::White);

    // 2. Clock Hands Setup (Width, Height)
    sf::RectangleShape hourHand(sf::Vector2f(8.f, 140.f));
    sf::RectangleShape minuteHand(sf::Vector2f(5.f, 200.f));
    sf::RectangleShape secondHand(sf::Vector2f(2.f, 220.f));

    // Set hands origin to their bottom-center so they rotate cleanly around the clock center
    hourHand.setOrigin(4.f, 140.f);
    minuteHand.setOrigin(2.5f, 200.f);
    secondHand.setOrigin(1.f, 220.f);

    hourHand.setPosition(center);
    minuteHand.setPosition(center);
    secondHand.setPosition(center);

    // Hand Colors
    hourHand.setFillColor(sf::Color(180, 180, 180));
    minuteHand.setFillColor(sf::Color(240, 240, 240));
    secondHand.setFillColor(sf::Color(255, 50, 50)); // Red second hand

    // Main Loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // --- Get System Time ---
        std::time_t currentTime = std::time(nullptr);
        std::tm* ltime = std::localtime(&currentTime);

        float hours = ltime->tm_hour;
        float minutes = ltime->tm_min;
        float seconds = ltime->tm_sec;

        // --- Calculate Angles (in degrees) ---
        // 360 degrees / 60 seconds = 6 degrees per second
        float secondAngle = seconds * 6.f; 
        
        // 6 degrees per minute + smooth movement fraction based on seconds
        float minuteAngle = (minutes * 6.f) + (seconds * 0.1f); 
        
        // 360 degrees / 12 hours = 30 degrees per hour + smooth movement fraction based on minutes
        float hourAngle = (hours * 30.f) + (minutes * 0.5f);

        // Apply rotation transforms
        secondHand.setRotation(secondAngle);
        minuteHand.setRotation(minuteAngle);
        hourHand.setRotation(hourAngle);

        // --- Rendering ---
        window.clear(sf::Color(30, 30, 30));

        window.draw(clockFace);
        
        // Draw ticks around the clock face
        for (int i = 0; i < 60; i++) {
            sf::RectangleShape tick(sf::Vector2f(i % 5 == 0 ? 4.f : 2.f, i % 5 == 0 ? 15.f : 8.f));
            tick.setOrigin(tick.getSize().x / 2.f, 0.f);
            tick.setPosition(center);
            tick.setFillColor(i % 5 == 0 ? sf::Color::White : sf::Color(100, 100, 100));
            
            // Math to offset ticks near the edge
            float angle = i * 6.f * PI / 180.f;
            tick.move(std::sin(angle) * 235.f, -std::cos(angle) * 235.f);
            tick.setRotation(i * 6.f);
            window.draw(tick);
        }

        // Draw hands
        window.draw(hourHand);
        window.draw(minuteHand);
        window.draw(secondHand);
        window.draw(centerPin);

        window.display();
    }

    return 0;
}

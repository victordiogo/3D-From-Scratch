// #ifndef _3D_FROM_SCRATCH_CAMERA_HPP
// #define _3D_FROM_SCRATCH_CAMERA_HPP

// class Camera {
// public:
//   struct Orientation {
//     vec3 front{};
//     vec3 up{};
//     vec3 right{};
//     double yaw{};
//     double pitch{};
//   };

//   enum Direction {
//     forward,
//     backward,
//     left,
//     right,
//   };

//   Camera(double vertical_fov, double aspect_ratio, double near_plane, double far_plane) noexcept;

//   using Time = double;
//   void move(Direction direction, Time seconds);

//   void rotate(double yaw, float pitch);

// private:
//   void update_orientation_vectors();

//   double m_vertical_fov{};
//   double m_aspect_ratio{};
//   double m_near_plane{};
//   double m_far_plane{};
//   double m_speed{ 1.0 };
//   vec3 m_position{};
//   Orientation m_orientation{};
// };

// #endif
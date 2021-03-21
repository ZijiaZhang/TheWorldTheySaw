// Header
#include "buttonStart.hpp"
#include "render.hpp"
#include "button.hpp"
#include "PhysicsObject.hpp"
#include <soldier.hpp>

std::map<ButtonType, std::string> Button::buttonNames = {
	{ButtonType::DEFAULT_BUTTON, "default"},
	{ButtonType::START, "start"},
	{ButtonType::LEVEL_SELECT, "level_select"},
	{ButtonType::QUIT, "quit"},
//    {ButtonType::SELECT_ROCKET, "select_rocket"},
//    {ButtonType::SELECT_AMMO, "select_ammo"},
//    {ButtonType::SELECT_LASER,"select_laser"},
//    {ButtonType::SELECT_BULLET, "select_bullet"},
//    {ButtonType::SELECT_DIRECT,"select_direct"},
//    {ButtonType::SELECT_A_STAR, "select_a_star"},
    {ButtonType::NEXT, "next"},
    {ButtonType::RESTART, "RESTART"},
    {ButtonType::RETURN, "return"},
    {ButtonType::LEVEL1, "level1"},
    {ButtonType::LEVEL2, "level2"},
	{ButtonType::LEVEL3, "level3"},
    {ButtonType::LEVEL4, "level4"},
    {ButtonType::LEVEL5, "level5"},
    {ButtonType::LEVEL6, "level6"}
    
};

ECS::Entity Button::createButton(ButtonType buttonType, vec2 position, COLLISION_HANDLER overlap)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = buttonNames[buttonType];
	ShadedMesh& resource = cache_resource(key);

	if (resource.effect.program.resource == 0)
	{
		std::string path = "/main scene/button_";
		path.append(key);
		path.append(".png");
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, textures_path(path), "textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;

	motion.scale = vec2({ 0.5f, 0.5f }) * static_cast<vec2>(resource.texture.size);

	// Update ZValuesMap in common
	motion.zValue = ZValuesMap["Fish"];

	auto& physics = entity.emplace<PhysicsObject>();
    physics.object_type = BUTTON;
    physics.attach(Overlap, overlap);

	// Create and (empty) Fish component to be able to refer to all fish
	auto& button = ECS::registry<Button>.emplace(entity);


	button.buttonType = buttonType;

	return entity;
}

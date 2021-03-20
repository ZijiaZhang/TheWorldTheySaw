// Header
#include "buttonStart.hpp"
#include "render.hpp"
#include "button.hpp"
#include "PhysicsObject.hpp"
#include <soldier.hpp>

std::map<ButtonIcon, std::string> Button::buttonNamesMap = {
	{ButtonIcon::DEFAULT_BUTTON, "default"},
	{ButtonIcon::START,          "start"},
	{ButtonIcon::LEVEL_SELECT,   "level_select"},
	{ButtonIcon::QUIT,           "quit"},
    {ButtonIcon::SELECT_ROCKET,  "select_rocket"},
    {ButtonIcon::SELECT_AMMO,    "select_ammo"},
    {ButtonIcon::SELECT_LASER,   "select_laser"},
    {ButtonIcon::SELECT_BULLET,  "select_bullet"},
    {ButtonIcon::SELECT_DIRECT,  "select_direct"},
    {ButtonIcon::SELECT_A_STAR,  "select_a_star"},
    {ButtonIcon::RETURN,         "return"},
    {ButtonIcon::LEVEL1,         "level1"},
    {ButtonIcon::LEVEL2,         "level2"},
	{ButtonIcon::LEVEL3,         "default"}
};


std::map<ButtonIcon, ButtonClass> Button::buttonClassMap = {
        {ButtonIcon::SELECT_ROCKET,  ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_AMMO,     ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_LASER,    ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_BULLET,   ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_DIRECT,   ButtonClass::ALGORITHM_SELECTION},
        {ButtonIcon::SELECT_A_STAR,  ButtonClass::ALGORITHM_SELECTION},
};

ECS::Entity Button::createButton(ButtonIcon buttonType, vec2 position, COLLISION_HANDLER overlap)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = buttonNamesMap[buttonType];
	ShadedMesh& resource = cache_resource(key);

	if (resource.effect.program.resource == 0)
	{
		std::string path = "/main scene/button_";
		path.append(key);
		path.append(".png");
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, textures_path(path), "shinning");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;

	motion.scale = vec2({ 1.0f, 1.0f }) * static_cast<vec2>(resource.texture.size);

	// Update ZValuesMap in common
	motion.zValue = ZValuesMap["Fish"];

	auto& physics = entity.emplace<PhysicsObject>();
    physics.object_type = BUTTON;
    physics.attach(Overlap, overlap);

	// Create and (empty) Fish component to be able to refer to all fish
	auto& button = ECS::registry<Button>.emplace(entity);


	button.buttonType = buttonType;
    button.buttonClass = buttonClassMap[buttonType];
	return entity;
}

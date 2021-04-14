// Header
#include "buttonStart.hpp"
#include "render.hpp"
#include "button.hpp"
#include "PhysicsObject.hpp"
#include <soldier.hpp>

std::map<ButtonIcon, std::string> Button::buttonNamesMap = {
	{ButtonIcon::DEFAULT_BUTTON, "default"},
	{ButtonIcon::START, "start"},
	{ButtonIcon::LEVEL_SELECT, "level_select"},
	{ButtonIcon::QUIT, "quit"},
    {ButtonIcon::SELECT_ROCKET, "select_rocket"},
    {ButtonIcon::SELECT_AMMO, "select_ammo"},
    {ButtonIcon::SELECT_LASER,"select_laser"},
    {ButtonIcon::SELECT_BULLET, "select_bullet"},
    {ButtonIcon::SELECT_DIRECT,"select_direct"},
    {ButtonIcon::SELECT_A_STAR, "select_a_star"},
    {ButtonIcon::SELECT_FIREBALL, "select_fireball"},
    {ButtonIcon::SELECT_FIELD, "select_field"},
    {ButtonIcon::NEXT, "next"},
    {ButtonIcon::RESTART, "RESTART"},
    {ButtonIcon::RETURN, "return"},
    {ButtonIcon::SAVE, "save"},
    {ButtonIcon::CONTINUE, "continue"},
    {ButtonIcon::LOCKED, "locked"},
    {ButtonIcon::TUTORIAL, "tutorial"},
    {ButtonIcon::SETTING, "setting"},
    {ButtonIcon::LEVEL1, "level1"},
    {ButtonIcon::LEVEL2, "level2"},
	{ButtonIcon::LEVEL3, "level3"},
    {ButtonIcon::LEVEL4, "level4"},
    {ButtonIcon::LEVEL5, "level5"},
    {ButtonIcon::LEVEL6, "level6"},
    {ButtonIcon::LEVEL7, "level7"},
    {ButtonIcon::LEVEL8, "level8"},
    {ButtonIcon::LEVEL9, "level9"},
    {ButtonIcon::LEVEL10, "level10"},
    {ButtonIcon::LEVEL11, "level11"},
    {ButtonIcon::LEVEL12, "level12"}
    
};


std::map<ButtonIcon, ButtonClass> Button::buttonClassMap = {
        {ButtonIcon::SELECT_ROCKET,  ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_AMMO,     ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_LASER,    ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_BULLET,   ButtonClass::WEAPON_SELECTION},
        {ButtonIcon::SELECT_DIRECT,   ButtonClass::ALGORITHM_SELECTION},
        {ButtonIcon::SELECT_A_STAR,  ButtonClass::ALGORITHM_SELECTION},
        {ButtonIcon::SELECT_FIREBALL, ButtonClass::MAGIC_SELECTION},
        {ButtonIcon::SELECT_FIELD, ButtonClass::MAGIC_SELECTION},
};


std::map<ButtonIcon, WeaponType> Button::weaponTypeMap = {
        {ButtonIcon::SELECT_ROCKET,  WeaponType::W_ROCKET},
        {ButtonIcon::SELECT_AMMO,     WeaponType::W_AMMO},
        {ButtonIcon::SELECT_LASER,    WeaponType::W_LASER},
        {ButtonIcon::SELECT_BULLET,   WeaponType::W_BULLET},
};

std::map<ButtonIcon, AIAlgorithm> Button::algorithmMap = {
        {ButtonIcon::SELECT_DIRECT,   AIAlgorithm::DIRECT},
        {ButtonIcon::SELECT_A_STAR,  AIAlgorithm::A_STAR},
};

std::map<ButtonIcon, MagicWeapon> Button::magicMap = {
        {ButtonIcon::SELECT_FIREBALL,   MagicWeapon::FIREBALL},
        {ButtonIcon::SELECT_FIELD,  MagicWeapon::FIELD},
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
    button.buttonClass = buttonClassMap[buttonType];
	return entity;
}

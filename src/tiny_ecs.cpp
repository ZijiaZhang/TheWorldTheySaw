// internal
#include "tiny_ecs.hpp"

#include <cassert>
#include <iostream>
#include <typeinfo>


// We store a list of all Component containers to be able to inspect the number of components and entities in each and to remove entities across containers
using namespace ECS;

std::vector<ContainerInterface*>& ContainerInterface::registry_list_singleton() {
	// This is a Meyer's singleton, i.e., a function returning a static local variable by reference to solve SIOF
	static std::vector<ContainerInterface*> singleton; // constructed during first call
	return singleton;
}

void ContainerInterface::clear_all_components() {
	for (auto reg : registry_list_singleton()) {
		reg->clear();
	}
}
void ContainerInterface::list_all_components() {
	// std::cout << "Debug info on all registry entries:\n";
	const auto& singleton = registry_list_singleton();
	for (auto reg : singleton) {
		assert(reg); // Must not be null
		if (reg->size() > 0) {
			/*
			std::cout
				<< "  " << reg->size() << " components of type "
				<< typeid(*reg).name() << "\n    ";
			*/
			
			for (auto entity : reg->entities) {
				// std::cout << entity.id << ", ";
			}
			// std::cout << '\n';
		}
	}
	// std::cout.flush();
}
void ContainerInterface::list_all_components_of(Entity e) {
	// std::cout << "Debug info on components of entity " << e.id << ":\n";
	for (auto reg : registry_list_singleton()) {
		assert(reg); // Must not be null
		if (reg->has(e)) {
			/*std::cout
				<< "  type " << typeid(*reg).name() << ", stored at location "
				<< reg->map_entity_component_index[e.id] << '\n';
			
			*/
			
		}
	}
}
void ContainerInterface::remove_all_components_of(Entity e) {
    if (e.has<ParentEntity>()){
        auto& parent = e.get<ParentEntity>();
        ECS::Entity parent_entity = parent.parent;
        e.remove<ParentEntity>();
        if (registry<ChildrenEntities>.has(parent_entity)){
            registry<ChildrenEntities>.get(parent_entity).children.erase(e);
        }
    }
    if(e.has<ChildrenEntities>()){
        auto& children = e.get<ChildrenEntities>().children;
        for(auto it= children.begin(); it!= children.end();) {
            remove_all_components_of(*it++);
        }
    }
	for (auto reg : registry_list_singleton()) {
		assert(reg); // Must not be null
		reg->remove(e);
	}
}

/*
* void ECS::Entity::attach(std::string key, std::function<void(ECS::Entity, ECS::Entity)> callback)
{
	collisionHandler.insert({ key, callback });
}

void ECS::Entity::update(std::string key, ECS::Entity e1, ECS::Entity e2)
{
	std::cout << "update\n";
	if (collisionHandler[key] != NULL)
		collisionHandler[key](e1, e2);
}
*/

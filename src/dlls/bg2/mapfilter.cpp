#include "cbase.h"
#include "mapfilter.h"
 
// Constructor
CMapEntityFilter::CMapEntityFilter() 
{
	keepList = new CUtlSortVector< const char *> (StrLessThan);
}
 
// Deconstructor
CMapEntityFilter::~CMapEntityFilter() 
{
	delete keepList;
}
 
// [bool] ShouldCreateEntity [char]
// Purpose   : Used to check if the passed in entity is on our stored list
// Arguments : The classname of an entity 
// Returns   : Boolean value - if we have it stored, we return false.
 
bool CMapEntityFilter::ShouldCreateEntity( const char *pClassname ) 
{
	//Check if the entity is in our keep list.
	if( keepList->Find( pClassname ) >= 0 )
		return false;
	else
		return true;
}
 
// [CBaseEntity] CreateNextEntity [char]
// Purpose   : Creates an entity
// Arguments : The classname of an entity
// Returns   : A pointer to the new entity
 
CBaseEntity* CMapEntityFilter::CreateNextEntity( const char *pClassname ) 
{
	return CreateEntityByName( pClassname);
}
 
// [void] AddKeep [char]
// Purpose   : Adds the passed in value to our list of items to keep
// Arguments : The class name of an entity
// Returns   : Void
 
void CMapEntityFilter::AddKeep( const char *sz) 
{
	keepList->Insert(sz);
}
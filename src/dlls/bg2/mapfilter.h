#include "cbase.h"
#include "mapentities.h"
#include "UtlSortVector.h"
 
#ifndef CMAPENTITYFILTER_H
#define CMAPENTITYFILTER_H
 
typedef const char* strptr;
 
static bool StrLessThan(  const strptr &src1,  const strptr &src2, void *pCtx ) 
{
	if( strcmp(src1, src2) >= 0)
		return false;
	else
		return true;
}
 
class CMapEntityFilter : public IMapEntityFilter
{
public:
	// constructor
	CMapEntityFilter();
	// deconstructor
	~CMapEntityFilter();
	
	// used to check if we should reset an entity or not
	virtual bool ShouldCreateEntity( const char *pClassname );
	// creates the next entity in our stored list.
	virtual CBaseEntity* CreateNextEntity( const char *pClassname );
	// add an entity to our list
	void AddKeep( const char*);
 
private:
	// our list of entities to keep
	CUtlSortVector< const char* > *keepList;
};
 
#endif 
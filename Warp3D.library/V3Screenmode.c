#include "w3d.h"

#include <proto/graphics.h>
#include <proto/picasso96.h>
/*
typedef struct
{
    ULONG ModeID;               // ModeID for OpenScreen
    ULONG Width, Height;        // Width and Height for visible screen
    ULONG Depth;                // Pixel Depth
    char DisplayName[DISPLAYNAMELEN];   // readable name of this display mode 
    W3D_Driver *Driver PACKED;  // Matching driver for this mode
    void *Next PACKED;          // Next W3D_ScreenMode in list
}
W3D_ScreenMode;

struct P96Mode {
	struct Node	Node;
	char			Description[MODENAMELENGTH];
	UWORD			Width;
	UWORD			Height;
	UWORD			Depth;
	ULONG			DisplayID;
};
*/
W3D_ScreenMode *W3D_GetScreenmodeList(void) {
    struct List *list;struct P96Mode *mode;W3D_ScreenMode *head=NULL,**tail=&head,*item;
    list=p96AllocModeListTags(TAG_DONE);if(!list)return NULL;
    for(mode=(struct P96Mode *)list->lh_Head;mode->Node.ln_Succ;mode=(struct P96Mode *)mode->Node.ln_Succ){
        int i;item=malloc(sizeof(*item));if(!item)break;
        item->ModeID=mode->DisplayID;item->Width=mode->Width;item->Height=mode->Height;item->Depth=mode->Depth;
        item->Driver=drivers[0];item->Next=NULL;
        for(i=0;i<DISPLAYNAMELEN-1&&i<MODENAMELENGTH-1&&mode->Description[i];++i)item->DisplayName[i]=mode->Description[i];
        item->DisplayName[i]=0;*tail=item;tail=(W3D_ScreenMode **)&item->Next;
    }
    p96FreeModeList(list);return head;
}

void W3D_FreeScreenmodeList(__REGA0(W3D_ScreenMode *list)) {
	W3D_ScreenMode* current;
	W3D_ScreenMode* next = list;
	LOG;
	while (next) {
		current = next;
		next = current->Next;
		free(current);
	}
}

ULONG W3D_BestModeID(__REGA0(struct TagItem *tags)) {
	int i;
	LOG;
	for (i = 0; tags[i].ti_Tag != TAG_DONE; ++i) {
		switch (tags[i].ti_Tag) {
		case W3D_BMI_DRIVER: tags[i].ti_Tag = TAG_IGNORE; break;
		case W3D_BMI_WIDTH: tags[i].ti_Tag = P96BIDTAG_NominalWidth; break;
		case W3D_BMI_HEIGHT: tags[i].ti_Tag = P96BIDTAG_NominalHeight; break;
		case W3D_BMI_DEPTH: tags[i].ti_Tag = P96BIDTAG_Depth; break;
		}
	}
	return p96BestModeIDTagList(tags);
}
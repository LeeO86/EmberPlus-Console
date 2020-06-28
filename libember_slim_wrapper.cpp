#include "libember_slim_wrapper.h"


static volatile int allocCount = 0;

void *allocMemoryImpl(size_t size)
{
    void* memPointer = ::operator new (size);
    allocCount++;
    return memPointer;
}

void freeMemoryImpl(void *pMemory)
{
    ::operator delete (pMemory);
    allocCount--;
}

void onThrowError(int error, pcstr pMessage)
{
    qDebug() << "libember_slim Error: " << error << " : " << pMessage;
}

void onFailAssertion(pcstr pFileName, int lineNumber)
{
    qDebug() << "Debug assertion failed @ " << pFileName << " line: " << lineNumber;
}

// ====================================================================
//
// Linked List
//
// ====================================================================

void ptrList_init(PtrList *pThis)
{
   bzero_item(*pThis);
}

void ptrList_addLast(PtrList *pThis, voidptr value)
{
   PtrListNode *pNode = newobj(PtrListNode);
   pNode->value = value;
   pNode->pNext = NULL;

   if(pThis->pLast == NULL)
   {
      pThis->pHead = pNode;
      pThis->pLast = pNode;
   }
   else
   {
      pThis->pLast->pNext = pNode;
      pThis->pLast = pNode;
   }

   pThis->count++;
}

void ptrList_free(PtrList *pThis)
{
   PtrListNode *pNode = pThis->pHead;
   PtrListNode *pPrev;

   while(pNode != NULL)
   {
      pPrev = pNode;
      pNode = pNode->pNext;

      freeMemory(pPrev);
   }

   bzero_item(*pThis);
}


// ====================================================================
//
// Model
//
// ====================================================================

static void element_init(Element *pThis, Element *pParent, GlowElementType type, berint number)
{
   bzero_item(*pThis);

   pThis->pParent = pParent;
   pThis->type = type;
   pThis->number = number;

   ptrList_init(&pThis->children);

   if(pParent != NULL)
      ptrList_addLast(&pParent->children, pThis);

   if(pThis->type == GlowElementType_Node)
      pThis->glow.node.isOnline = true;
}

static void element_free(Element *pThis)
{
   Element *pChild;
   Target *pTarget;
   PtrListNode *pNode;

   for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
   {
      pChild = (Element *)pNode->value;
      element_free(pChild);
      freeMemory(pChild);
   }

   ptrList_free(&pThis->children);

   if(pThis->type == GlowElementType_Parameter)
   {
      glowValue_free(&pThis->glow.param.value);

      if(pThis->glow.param.pEnumeration != NULL)
         freeMemory((void *)pThis->glow.param.pEnumeration);
      if(pThis->glow.param.pFormula != NULL)
         freeMemory((void *)pThis->glow.param.pFormula);
      if(pThis->glow.param.pFormat != NULL)
         freeMemory((void *)pThis->glow.param.pFormat);
   }
   else if(pThis->type == GlowElementType_Matrix)
   {
      for(pNode = pThis->glow.matrix.targets.pHead; pNode != NULL; pNode = pNode->pNext)
      {
         pTarget = (Target *)pNode->value;

         if(pTarget->pConnectedSources != NULL)
            freeMemory(pTarget->pConnectedSources);

         freeMemory(pTarget);
      }

      for(pNode = pThis->glow.matrix.sources.pHead; pNode != NULL; pNode = pNode->pNext)
      {
         if(pNode->value != NULL)
            freeMemory(pNode->value);
      }

      ptrList_free(&pThis->glow.matrix.targets);
      ptrList_free(&pThis->glow.matrix.sources);
   }
   else if(pThis->type == GlowElementType_Function)
   {
      glowFunction_free(&pThis->glow.function);
   }
   else if(pThis->type == GlowElementType_Node)
   {
      glowNode_free(&pThis->glow.node);
   }

   bzero_item(*pThis);
}

static pcstr element_getIdentifier(const Element *pThis)
{
   switch(pThis->type)
   {
      case GlowElementType_Node: return pThis->glow.node.pIdentifier;
      case GlowElementType_Parameter: return pThis->glow.param.pIdentifier;
      case GlowElementType_Matrix: return pThis->glow.matrix.matrix.pIdentifier;
      case GlowElementType_Function: return pThis->glow.function.pIdentifier;
    default: break;
   }

   return NULL;
}

static berint *element_getPath(const Element *pThis, berint *pBuffer, int *pCount)
{
   const Element *pElement;
   berint *pPosition = &pBuffer[*pCount];
   int count = 0;

   for(pElement = pThis; pElement->pParent != NULL; pElement = pElement->pParent)
   {
      pPosition--;

      if(pPosition < pBuffer)
         return NULL;

      *pPosition = pElement->number;
      count++;

   }

   *pCount = count;
   return pPosition;
}

static pcstr element_getIdentifierPath(const Element *pThis, pstr pBuffer, int bufferSize)
{
   const Element *pElement;
   pstr pPosition = &pBuffer[bufferSize - 1];
   int length;
   pcstr pIdentifier;

   *pPosition = 0;

   for(pElement = pThis; pElement->pParent != NULL; pElement = pElement->pParent)
   {
      if(*pPosition != 0)
         *--pPosition = '/';

      pIdentifier = element_getIdentifier(pElement);
      length = (int)strlen(pIdentifier);
      pPosition -= length;

      if(pPosition < pBuffer)
         return NULL;

      memcpy(pPosition, pIdentifier, length);
   }

   return pPosition;
}

static Element *element_findChild(const Element *pThis, berint number)
{
   Element *pChild;
   PtrListNode *pNode;

   for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
   {
      pChild = (Element *)pNode->value;

      if(pChild->number == number)
         return pChild;
   }

   return NULL;
}

static Element *element_findChildByIdentifier(const Element *pThis, pcstr pIdentifier)
{
   Element *pChild;
   pcstr pIdent;
   PtrListNode *pNode;

   for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
   {
      pChild = (Element *)pNode->value;
      pIdent = element_getIdentifier(pChild);

      if(strcmp(pIdent, pIdentifier) == 0)
         return pChild;
   }

   return NULL;
}

static Element *element_findDescendant(const Element *pThis, const berint *pPath, int pathLength, Element **ppParent)
{
   int index;
   Element *pElement = (Element *)pThis;
   Element *pParent = NULL;

   for(index = 0; index < pathLength; index++)
   {
      if(pElement != NULL)
         pParent = pElement;

      pElement = element_findChild(pElement, pPath[index]);

      if(pElement == NULL)
      {
         if(index < pathLength - 1)
            pParent = NULL;

         break;
      }
   }

   if(ppParent != NULL)
      *ppParent = pParent;

   return pElement;
}

static GlowParameterType element_getParameterType(const Element *pThis)
{
   if(pThis->type == GlowElementType_Parameter)
   {
      if(pThis->paramFields & GlowFieldFlag_Enumeration)
         return GlowParameterType_Enum;

      if(pThis->paramFields & GlowFieldFlag_Value)
         return pThis->glow.param.value.flag;

      if(pThis->paramFields & GlowFieldFlag_Type)
         return pThis->glow.param.type;
   }

   return (GlowParameterType)0;
}

static Target *element_findOrCreateTarget(Element *pThis, berint number)
{
   Target *pTarget;
   PtrListNode *pNode;

   if(pThis->type == GlowElementType_Matrix)
   {
      for(pNode = pThis->glow.matrix.targets.pHead; pNode != NULL; pNode = pNode->pNext)
      {
         pTarget = (Target *)pNode->value;

         if(pTarget->number == number)
            return pTarget;
      }

      pTarget = newobj(Target);
      bzero_item(*pTarget);
      pTarget->number = number;

      ptrList_addLast(&pThis->glow.matrix.targets, pTarget);
      return pTarget;
   }

   return NULL;
}

static Source *element_findSource(const Element *pThis, berint number)
{
   Source *pSource;
   PtrListNode *pNode;

   if(pThis->type == GlowElementType_Matrix)
   {
      for(pNode = pThis->glow.matrix.sources.pHead; pNode != NULL; pNode = pNode->pNext)
      {
         pSource = (Source *)pNode->value;

         if(pSource->number == number)
            return pSource;
      }
   }

   return NULL;
}

static void printValue(const GlowValue *pValue)
{
   switch(pValue->flag)
   {
      case GlowParameterType_Integer:
         qDebug("integer '%lld'", pValue->choice.integer);
         break;

      case GlowParameterType_Real:
         qDebug("real '%lf'", pValue->choice.real);
         break;

      case GlowParameterType_String:
         qDebug("string '%s'", pValue->choice.pString);
         break;

      case GlowParameterType_Boolean:
         qDebug("boolean '%d'", pValue->choice.boolean);
         break;

      case GlowParameterType_Octets:
         qDebug("octets length %d", pValue->choice.octets.length);
         break;

      default:
         break;
   }
}

static QString returnValue(const GlowValue *pValue)
{
    QString val, octet;
    switch(pValue->flag)
    {
    case GlowParameterType_Integer:
        val = QString::asprintf("%lld", pValue->choice.integer);
        break;

    case GlowParameterType_Real:
        val = QString::asprintf("%lf", pValue->choice.real);
        break;

    case GlowParameterType_String:
        val = QString::asprintf("%s", pValue->choice.pString);
        break;

    case GlowParameterType_Boolean:
        val = QString::asprintf("%d", pValue->choice.boolean);
        break;

    case GlowParameterType_Octets:
        for(int i = 0; i < pValue->choice.octets.length; i++){
            octet = QString::asprintf("%02hhX ", pValue->choice.octets.pOctets[i]);
            val.append(octet);
        }
        break;

    default:
        break;
    }
    return val;
}

static void printMinMax(const GlowMinMax *pMinMax)
{
   switch(pMinMax->flag)
   {
      case GlowParameterType_Integer:
         qDebug("%lld", pMinMax->choice.integer);
         break;

      case GlowParameterType_Real:
         qDebug("%lf", pMinMax->choice.real);
         break;

      default:
         break;
   }
}

static void element_print(const Element *pThis, bool isVerbose)
{
   GlowFieldFlags fields;
   const GlowParameter *pParameter;
   const GlowMatrix *pMatrix;
   const GlowFunction *pFunction;
   int index;

   if(pThis->type == GlowElementType_Parameter)
   {
      pParameter = &pThis->glow.param;
      qDebug("P %04d %s\n", pThis->number, pParameter->pIdentifier);

      if(isVerbose)
      {
         fields = pThis->paramFields;

         if(fields & GlowFieldFlag_Description)
            qDebug("  description:      %s\n", pParameter->pDescription);

         if(fields & GlowFieldFlag_Value)
         {
            qDebug("  value:            ");
            printValue(&pParameter->value);
            qDebug("\n");
         }

         if(fields & GlowFieldFlag_Minimum)
         {
            qDebug("  minimum:          ");
            printMinMax(&pParameter->minimum);
            qDebug("\n");
         }

         if(fields & GlowFieldFlag_Maximum)
         {
            qDebug("  maximum:          ");
            printMinMax(&pParameter->maximum);
            qDebug("\n");
         }

         if(fields & GlowFieldFlag_Access)
            qDebug("  access:           %d\n", pParameter->access);
         if(fields & GlowFieldFlag_Factor)
            qDebug("  factor:           %d\n", pParameter->factor);
         if(fields & GlowFieldFlag_IsOnline)
            qDebug("  isOnline:         %d\n", pParameter->isOnline);
         if(fields & GlowFieldFlag_Step)
            qDebug("  step:             %d\n", pParameter->step);
         if(fields & GlowFieldFlag_Type)
            qDebug("  type:             %d\n", pParameter->type);
         if(fields & GlowFieldFlag_StreamIdentifier)
            qDebug("  streamIdentifier: %d\n", pParameter->streamIdentifier);

         if(fields & GlowFieldFlag_StreamDescriptor)
         {
            qDebug("  streamDescriptor:\n");
            qDebug("    format:         %d\n", pParameter->streamDescriptor.format);
            qDebug("    offset:         %d\n", pParameter->streamDescriptor.offset);
         }

         if(pParameter->pEnumeration != NULL)
            qDebug("  enumeration:\n%s\n", pParameter->pEnumeration);
         if(pParameter->pFormat != NULL)
            qDebug("  format:           %s\n", pParameter->pFormat);
         if(pParameter->pFormula != NULL)
            qDebug("  formula:\n%s\n", pParameter->pFormula);
         if(pParameter->pSchemaIdentifiers != NULL)
            qDebug("  schemaIdentifiers: %s\n", pParameter->pSchemaIdentifiers);
      }
   }
   else if(pThis->type == GlowElementType_Node)
   {
      qDebug("N %04d %s\n", pThis->number, pThis->glow.node.pIdentifier);

      if(isVerbose)
      {
         qDebug("  description:      %s\n", pThis->glow.node.pDescription);
         qDebug("  isRoot:           %s\n", pThis->glow.node.isRoot ? "true" : "false");
         qDebug("  isOnline:         %s\n", pThis->glow.node.isOnline ? "true" : "false");

         if(pThis->glow.node.pSchemaIdentifiers != NULL)
            qDebug("  schemaIdentifiers: %s\n", pThis->glow.node.pSchemaIdentifiers);
      }
   }
   else if(pThis->type == GlowElementType_Matrix)
   {
      pMatrix = &pThis->glow.matrix.matrix;
      qDebug("M %04d %s\n", pThis->number, pMatrix->pIdentifier);

      if(isVerbose)
      {
         if(pMatrix->pDescription != NULL)
            qDebug("  description:                 %s\n", pMatrix->pDescription);
         if(pMatrix->type != GlowMatrixType_OneToN)
            qDebug("  type:                        %d\n", pMatrix->type);
         if(pMatrix->addressingMode != GlowMatrixAddressingMode_Linear)
            qDebug("  addressingMode:              %d\n", pMatrix->addressingMode);
         qDebug("  targetCount:                 %d\n", pMatrix->targetCount);
         qDebug("  sourceCount:                 %d\n", pMatrix->sourceCount);
         if(pMatrix->maximumTotalConnects != 0)
            qDebug("  maximumTotalConnects:        %d\n", pMatrix->maximumTotalConnects);
         if(pMatrix->maximumConnectsPerTarget != 0)
            qDebug("  maximumConnectsPerTarget:    %d\n", pMatrix->maximumConnectsPerTarget);
         if(glowParametersLocation_isValid(&pMatrix->parametersLocation))
         {
            qDebug("  parametersLocation:          ");

            if(pMatrix->parametersLocation.kind == GlowParametersLocationKind_BasePath)
            {
               for(index = 0; index < pMatrix->parametersLocation.choice.basePath.length; index++)
               {
                  qDebug("%d", pMatrix->parametersLocation.choice.basePath.ids[index]);

                  if(index < pMatrix->parametersLocation.choice.basePath.length - 1)
                     qDebug(".");
               }

               qDebug("\n");
            }
            else if(pMatrix->parametersLocation.kind == GlowParametersLocationKind_Inline)
            {
               qDebug("%d\n", pMatrix->parametersLocation.choice.inlineId);
            }
         }
         if(pMatrix->pSchemaIdentifiers != NULL)
            qDebug("  schemaIdentifiers:            %s\n", pMatrix->pSchemaIdentifiers);
      }
   }
   else if(pThis->type == GlowElementType_Function)
   {
      pFunction = &pThis->glow.function;
      qDebug("F %04d %s\n", pThis->number, pFunction->pIdentifier);

      if(isVerbose)
      {
         if(pFunction->pDescription != NULL)
            qDebug("  description:                 %s\n", pFunction->pDescription);
         if(pFunction->pArguments != NULL)
         {
            qDebug("  arguments:\n");
            for(index = 0; index < pFunction->argumentsLength; index++)
               qDebug("    %s:%d\n", pFunction->pArguments[index].pName, pFunction->pArguments[index].type);
         }
         if(pFunction->pResult != NULL)
         {
            qDebug("  result:\n");
            for(index = 0; index < pFunction->resultLength; index++)
               qDebug("    %s:%d\n", pFunction->pResult[index].pName, pFunction->pResult[index].type);
         }
      }
   }
}


// ====================================================================
//
// glow handlers
//
// ====================================================================

void libember_slim_wrapper::onNode(const GlowNode *pNode, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state)
{
    Session *pSession = (Session *)state;
    Element *pElement;
    Element *pParent;
    //qDebug() << "recieved Node";
    // if received element is a child of current cursor, print it
    //if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
    //                        && pathLength == pSession->cursorPathLength + 1){
    //    qDebug("* N %04d %s\n", pPath[pathLength - 1], pNode->pIdentifier);
    //}

    pElement = element_findDescendant(&pSession->root, pPath, pathLength, &pParent);

    if(pParent != NULL)
    {
        if(pElement == NULL)
        {
            pElement = newobj(Element);
            element_init(pElement, pParent, GlowElementType_Node, pPath[pathLength - 1]);

            if(fields & GlowFieldFlag_Identifier)
                pElement->glow.node.pIdentifier = strdup(pNode->pIdentifier);
        }

        if(fields & GlowFieldFlag_Description)
            pElement->glow.node.pDescription = strdup(pNode->pDescription);

        if(fields & GlowFieldFlag_IsOnline)
            pElement->glow.node.isOnline = pNode->isOnline;

        if(fields & GlowFieldFlag_IsRoot)
            pElement->glow.node.isRoot = pNode->isRoot;

        if(fields & GlowFieldFlag_SchemaIdentifier)
            pElement->glow.node.pSchemaIdentifiers = strdup(pNode->pSchemaIdentifiers);
    }
    pSession->obj->nodeReturned(pElement);
}

void libember_slim_wrapper::onParameter(const GlowParameter *pParameter, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   Element *pParent;
   GlowParameter *pLocalParam;
    //qDebug() << "recieved Parameter";
   // if received element is a child of current cursor, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength + 1)
      qDebug("* P %04d %s\n", pPath[pathLength - 1], pParameter->pIdentifier);

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, &pParent);

   if(pParent != NULL)
   {
      if(pElement == NULL)
      {
         pElement = newobj(Element);
         element_init(pElement, pParent, GlowElementType_Parameter, pPath[pathLength - 1]);
      }

      pLocalParam = &pElement->glow.param;

      if((fields & GlowFieldFlag_Identifier) == GlowFieldFlag_Identifier)
         pLocalParam->pIdentifier = strdup(pParameter->pIdentifier);
      if(fields & GlowFieldFlag_Description)
         pLocalParam->pDescription = strdup(pParameter->pDescription);
      if(fields & GlowFieldFlag_Value)
      {
         glowValue_free(&pLocalParam->value);
         glowValue_copyTo(&pParameter->value, &pLocalParam->value);
      }
      if(fields & GlowFieldFlag_Minimum)
         memcpy(&pLocalParam->minimum, &pParameter->minimum, sizeof(GlowMinMax));
      if(fields & GlowFieldFlag_Maximum)
         memcpy(&pLocalParam->maximum, &pParameter->maximum, sizeof(GlowMinMax));
      if(fields & GlowFieldFlag_Access)
         pLocalParam->access = pParameter->access;
      if(fields & GlowFieldFlag_Factor)
         pLocalParam->factor = pParameter->factor;
      if(fields & GlowFieldFlag_IsOnline)
         pLocalParam->isOnline = pParameter->isOnline;
      if(fields & GlowFieldFlag_Step)
         pLocalParam->step = pParameter->step;
      if(fields & GlowFieldFlag_Type)
         pLocalParam->type = pParameter->type;
      if(fields & GlowFieldFlag_StreamIdentifier)
         pLocalParam->streamIdentifier = pParameter->streamIdentifier;
      if(fields & GlowFieldFlag_StreamDescriptor)
         memcpy(&pLocalParam->streamDescriptor, &pParameter->streamDescriptor, sizeof(GlowStreamDescription));
      if(fields & GlowFieldFlag_SchemaIdentifier)
         pLocalParam->pSchemaIdentifiers = strdup(pParameter->pSchemaIdentifiers);

      if(pSession->pEnumeration != NULL)
      {
         pLocalParam->pEnumeration = pSession->pEnumeration;
         pSession->pEnumeration = NULL;
      }
      if(pSession->pFormat != NULL)
      {
         pLocalParam->pFormat = pSession->pFormat;
         pSession->pFormat = NULL;
      }
      if(pSession->pFormula != NULL)
      {
         pLocalParam->pFormula = pSession->pFormula;
         pSession->pFormula = NULL;
      }

      pElement->paramFields = (GlowFieldFlags)(pElement->paramFields | fields);

      // if cursor parameter has updated value, print value
      if(pElement == pSession->pCursor
      && (fields & GlowFieldFlag_Value))
      {
         printValue(&pParameter->value);
         qDebug("\n");
      }
   }
}

void libember_slim_wrapper::onMatrix(const GlowMatrix *pMatrix, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   Element *pParent;
    qDebug() << "recieved Matrix";
   // if received element is a child of current cursor, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength + 1)
      qDebug("* M %04d %s\n", pPath[pathLength - 1], pMatrix->pIdentifier);

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, &pParent);

   if(pParent != NULL)
   {
      if(pElement == NULL)
      {
         pElement = newobj(Element);
         element_init(pElement, pParent, GlowElementType_Matrix, pPath[pathLength - 1]);
      }

      memcpy(&pElement->glow.matrix.matrix, pMatrix, sizeof(*pMatrix));
      pElement->glow.matrix.matrix.pIdentifier = strdup(pMatrix->pIdentifier);
      pElement->glow.matrix.matrix.pDescription = strdup(pMatrix->pDescription);
      pElement->glow.matrix.matrix.pSchemaIdentifiers = strdup(pMatrix->pSchemaIdentifiers);
   }
}

void libember_slim_wrapper::onTarget(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
    qDebug() << "recieved Target";
   // if signal resides in cursor element, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength)
      qDebug("* T %04d\n", pSignal->number);

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, NULL);

   if(pElement != NULL
   && pElement->type == GlowElementType_Matrix)
      element_findOrCreateTarget(pElement, pSignal->number);
}

void libember_slim_wrapper::onSource(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   Source *pSource;
    qDebug() << "recieved Source";
   // if signal resides in cursor element, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength)
      qDebug("* S %04d\n", pSignal->number);

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, NULL);

   if(pElement != NULL
   && pElement->type == GlowElementType_Matrix)
   {
      pSource = newobj(Source);
      bzero_item(*pSource);
      pSource->number = pSignal->number;

      ptrList_addLast(&pElement->glow.matrix.sources, pSource);
   }
}

void libember_slim_wrapper::onConnection(const GlowConnection *pConnection, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   int index;
   Target *pTarget;
    qDebug() << "recieved Connection";
   // if signal resides in cursor element, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength)
   {
      qDebug("* C %04d <- [", pConnection->target);

      for(index = 0; index < pConnection->sourcesLength; index++)
      {
         qDebug("%04d", pConnection->pSources[index]);

         if(index < pConnection->sourcesLength - 1)
            qDebug(", ");
      }

      qDebug("]\n");
   }

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, NULL);

   if(pElement != NULL
   && pElement->type == GlowElementType_Matrix)
   {
      pTarget = element_findOrCreateTarget(pElement, pConnection->target);

      if(pTarget != NULL)
      {
         if(pTarget->pConnectedSources != NULL)
            freeMemory(pTarget->pConnectedSources);

         pTarget->pConnectedSources = newarr(berint, pConnection->sourcesLength);
         memcpy(pTarget->pConnectedSources, pConnection->pSources, pConnection->sourcesLength * sizeof(berint));
         pTarget->connectedSourcesCount = pConnection->sourcesLength;
      }
   }
}

static void cloneTupleItemDescription(GlowTupleItemDescription *pDest, const GlowTupleItemDescription *pSource)
{
   memcpy(pDest, pSource, sizeof(*pSource));
   pDest->pName = strdup(pSource->pName);
}

void libember_slim_wrapper::onFunction(const GlowFunction *pFunction, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   Element *pParent;
   int index;
    qDebug() << "recieved Function";
   // if received element is a child of current cursor, print it
   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
   && pathLength == pSession->cursorPathLength + 1)
      qDebug("* F %04d %s\n", pPath[pathLength - 1], pFunction->pIdentifier);

   pElement = element_findDescendant(&pSession->root, pPath, pathLength, &pParent);

   if(pParent != NULL)
   {
      if(pElement == NULL)
      {
         pElement = newobj(Element);
         element_init(pElement, pParent, GlowElementType_Function, pPath[pathLength - 1]);
      }

      memcpy(&pElement->glow.function, pFunction, sizeof(*pFunction));
      pElement->glow.function.pIdentifier = strdup(pFunction->pIdentifier);
      pElement->glow.function.pDescription = strdup(pFunction->pDescription);

      // clone arguments
      if(pFunction->pArguments != NULL)
      {
         pElement->glow.function.pArguments = newarr(GlowTupleItemDescription, pFunction->argumentsLength);

         for(index = 0; index < pFunction->argumentsLength; index++)
            cloneTupleItemDescription(&pElement->glow.function.pArguments[index], &pFunction->pArguments[index]);
      }

      // clone result
      if(pFunction->pResult != NULL)
      {
         pElement->glow.function.pResult = newarr(GlowTupleItemDescription, pFunction->resultLength);

         for(index = 0; index < pFunction->resultLength; index++)
            cloneTupleItemDescription(&pElement->glow.function.pResult[index], &pFunction->pResult[index]);
      }
   }
}

void libember_slim_wrapper::onInvocationResult(const GlowInvocationResult *pInvocationResult, voidptr state)
{
    Q_UNUSED(state);
   int index;
   pcstr status = pInvocationResult->hasError
                  ? "error"
                  : "ok";

   qDebug("* IR %04d %s\n", pInvocationResult->invocationId, status);

   if(pInvocationResult->hasError == false)
   {
      for(index = 0; index < pInvocationResult->resultLength; index++)
      {
         qDebug("    ");
         printValue(&pInvocationResult->pResult[index]);
         qDebug("\n");
      }
   }
}

void libember_slim_wrapper::onOtherPackageReceived(const byte *pPackage, int length, voidptr state)
{
   Session *pSession = (Session *)state;
   byte buffer[16];
   unsigned int txLength;

   if(length >= 4
   && pPackage[1] == EMBER_MESSAGE_ID
   && pPackage[2] == EMBER_COMMAND_KEEPALIVE_REQUEST)
   {
       qDebug() << "recieved KeepAlive-Request";
      txLength = emberFraming_writeKeepAliveResponse(buffer, sizeof(buffer), pPackage[0]);
      pSession->obj->send(QByteArray((char *)buffer, txLength));
   }
}

void libember_slim_wrapper::onUnsupportedTltlv(const BerReader *pReader, const berint *pPath, int pathLength, GlowReaderPosition position, voidptr state)
{
    Q_UNUSED(pPath);
    Q_UNUSED(pathLength);
   Session *pSession = (Session *)state;
    qDebug() << "recieved UnsupportedTltlv";
   if(position == GlowReaderPosition_ParameterContents)
   {
      if(berTag_equals(&pReader->tag, &glowTags.parameterContents.enumeration))
      {
         pSession->pEnumeration = newarr(char, pReader->length + 1);
         berReader_getString(pReader, pSession->pEnumeration, pReader->length);
      }
      else if(berTag_equals(&pReader->tag, &glowTags.parameterContents.formula))
      {
         pSession->pFormula = newarr(char, pReader->length + 1);
         berReader_getString(pReader, pSession->pFormula, pReader->length);
      }
      else if(berTag_equals(&pReader->tag, &glowTags.parameterContents.format))
      {
         pSession->pFormat = newarr(char, pReader->length + 1);
         berReader_getString(pReader, pSession->pFormat, pReader->length);
      }
   }
}

void libember_slim_wrapper::onLastPackageRecieved(const byte *pPackage, int length, voidptr state)
{
    Q_UNUSED(pPackage);
    Q_UNUSED(length);
    Session *pSession = (Session *)state;
    qDebug() << "recieved LastPackage";

}


libember_slim_wrapper::libember_slim_wrapper(QObject *parent) : QObject(parent)
{
    ember_init(onThrowError, onFailAssertion, allocMemoryImpl, freeMemoryImpl);
    tcpSock = new QTcpSocket(this);
    tcpSock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(tcpSock, &QIODevice::readyRead, this, &libember_slim_wrapper::readSocket);
    connect(tcpSock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &libember_slim_wrapper::socketError);
    readingTimeOut = new QTimer(this);
    connect(readingTimeOut, &QTimer::timeout, this, &libember_slim_wrapper::runFinished);
    readingTimeOut->setSingleShot(true);
}

libember_slim_wrapper::~libember_slim_wrapper()
{
    qDebug() << "Destruct libember_slim_wrapper";
    if(m_session.root.type == GlowElementType_Node){
        element_free(&m_session.root);
    }
    delete readingTimeOut;
    delete tcpSock;
}

void libember_slim_wrapper::connectEmber(QUrl url, int timeOut)
{
    QString host = url.host();
    int port = url.port(EMBER_DEF_PORT);
    readingTimeOut->setInterval(timeOut);
    tcpSock->connectToHost(host, port);

    bzero_item(m_session);
    m_session.obj = this;
    m_session.pCursor = &m_session.root;
    m_session.remoteAddress = host.toStdString().c_str();
    m_session.remotePort = port;
    element_init(&m_session.root, NULL, GlowElementType_Node, 0);

    if(tcpSock->waitForConnected(timeOut*5)){
        qDebug() << "Connected to: " << url.toString();
        const int rxBufferSize = 1290; // max size of unescaped package
        pRxBuffer = newarr(byte, rxBufferSize);
        glowReader_init(&p_reader, onNode, onParameter, NULL, NULL, (voidptr)&m_session, pRxBuffer, rxBufferSize);
        p_reader.base.onMatrix = onMatrix;
        p_reader.base.onTarget = onTarget;
        p_reader.base.onSource = onSource;
        p_reader.base.onConnection = onConnection;
        p_reader.base.onFunction = onFunction;
        p_reader.base.onInvocationResult = onInvocationResult;
        p_reader.onOtherPackageReceived = onOtherPackageReceived;
        p_reader.base.onUnsupportedTltlv = onUnsupportedTltlv;
        p_reader.onLastPackageReceived = onLastPackageRecieved;
    }

}

void libember_slim_wrapper::walkTree()
{
    m_walk = true;
    getDirectory(&m_session.root);
    readingTimeOut->start();
}

void libember_slim_wrapper::send(QByteArray msg)
{
    if(tcpSock->isOpen() && tcpSock->isWritable()){
        //qDebug() << "writing to Socket: " << msg;
        tcpSock->write(msg);
    }
}

void libember_slim_wrapper::getDirectory(Element *pElement)
{
    GlowOutput output;
    const int bufferSize = 512;
    byte *pBuffer;
    GlowCommand command;
    pBuffer = newarr(byte, bufferSize);
    berint pathBuffer[GLOW_MAX_TREE_DEPTH];
    int pathLength = GLOW_MAX_TREE_DEPTH;
    berint *pPath = element_getPath(pElement, pathBuffer, &pathLength);

    bzero_item(command);
    command.number = GlowCommandType_GetDirectory;
    glowOutput_init(&output, pBuffer, bufferSize, 0);
    glowOutput_beginPackage(&output, true);
    glow_writeQualifiedCommand(
            &output,
            &command,
            pPath,
            pathLength,
            pElement->type);
    send(QByteArray((char *)pBuffer, glowOutput_finishPackage(&output)));

    freeMemory(pBuffer);
}

void libember_slim_wrapper::callChild(Element *pElement)
{
    if(pElement != NULL)
    {
         m_session.cursorPathLength = GLOW_MAX_TREE_DEPTH;
         m_session.pCursorPath = element_getPath(pElement, m_session.cursorPathBuffer, &m_session.cursorPathLength);
         m_session.pCursor = pElement;
    }
    getDirectory(m_session.pCursor);
}

void libember_slim_wrapper::nodeReturned(Element *pElement)
{
    if(m_walk){
        getDirectory(pElement);
        readingTimeOut->start();
    }
}

void libember_slim_wrapper::findParams(Element *pStart)
{
    Element *pThis = pStart;
    if(pThis != NULL) {
        if(pThis->type == GlowElementType_Parameter){           // <------ TODO: add other types for PrintOut
            writeParam(pThis);
        } else {
            for(int i = 1; i <= pThis->children.count ; i++){
                Element *child = element_findChild(pThis, i);
                if(child != NULL)
                        findParams(child);
            }
        }
    }
}

void libember_slim_wrapper::writeParam(Element *param)
{
    QString numPath, identPath, val;
    int pathLength = GLOW_MAX_TREE_DEPTH;
    berint pathBuffer[GLOW_MAX_TREE_DEPTH];
    berint *path = element_getPath(param, pathBuffer, &pathLength);
    for(int i = 0; i < pathLength; i++){
        numPath.append(QString::number(path[i])+".");
    }
    QString numPathFixed = numPath.left(numPath.lastIndexOf(QChar('.'))).append("/>");
    char identPathBuffer[IDENT_PATH_BUFFER];
    identPath = element_getIdentifierPath(param, identPathBuffer, IDENT_PATH_BUFFER);
    identPath.append("/>");
    val = returnValue(&param->glow.param.value);
    qDebug() <<  numPathFixed << " | " << identPath << val;
    if (m_numPathOut)
                output.append(numPathFixed+val);
    else        output.append(identPath+val);

}

void libember_slim_wrapper::readSocket()
{
    QByteArray newData = tcpSock->readAll();
    if(newData.isEmpty())
        return;
    byte *pNewData = (byte *) newData.data();
    //qDebug() << "reading from Socket: " << newData << " | " << newData.size();
    glowReader_readBytes(&p_reader, pNewData, newData.size());
}

void libember_slim_wrapper::socketError(QAbstractSocket::SocketError socketError)
{
    QString msg;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        msg.append("TCP-Socket Closed: " + tcpSock->errorString());
        break;
    case QAbstractSocket::HostNotFoundError:
        msg.append("TCP-Socket 404 Error: " + tcpSock->errorString());
        break;
    case QAbstractSocket::ConnectionRefusedError:
        msg.append("TCP-Socket Error: " + tcpSock->errorString());
        break;
    default:
        msg.append("TCP-Socket Error: " + tcpSock->errorString());
    }
    emit error(1, msg);
}

void libember_slim_wrapper::runFinished()
{
    tcpSock->disconnectFromHost();

    findParams(&m_session.root);

    emit finishedEmber(output);
}

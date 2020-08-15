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

static QString printValue(const GlowValue *pValue)
{
    QString val, octet;
    switch(pValue->flag)
    {
    case GlowParameterType_Integer:
       val = QString::asprintf("integer '%lld'", pValue->choice.integer);
       break;

    case GlowParameterType_Real:
       val = QString::asprintf("real '%lf'", pValue->choice.real);
       break;

    case GlowParameterType_String:
       val = QString::asprintf("string '%s'", pValue->choice.pString);
       break;

    case GlowParameterType_Boolean:
       val = QString::asprintf("boolean '%s'", pValue->choice.boolean ? "true" : "false");
       break;

    case GlowParameterType_Octets:
       val = QString::asprintf("octets 'length %d' ", pValue->choice.octets.length);
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

static QString printMinMax(const GlowMinMax *pMinMax)
{
    QString ret;
    switch(pMinMax->flag)
    {
        case GlowParameterType_Integer:
            ret = QString::asprintf("%lld", pMinMax->choice.integer);
            break;

        case GlowParameterType_Real:
            ret = QString::asprintf("%lf", pMinMax->choice.real);
            break;

        default:
            break;
    }
    return ret;
}

static QStringList printEnum(const GlowParameter *param)
{
    QStringList ret;
    ret =  QString(param->pEnumeration).split("\n");
    for (int i = 0; i < ret.size(); i++) {
        ret[i] = QString("  |   |-- %1: ").arg(i) + ret[i];
    }
    return ret;
}

static QString printAccess(const EGlowAccess acc)
{
    QString ret;
    switch (acc) {
    case GlowAccess_None:
        ret = "None";
        break;
    case GlowAccess_Read:
        ret = "Read";
        break;
    case GlowAccess_Write:
        ret = "Write";
        break;
    case GlowAccess_ReadWrite:
        ret = "Read & Write";
        break;
    }
    return ret;
}

static QStringList element_print(const Element *pThis)
{
    QStringList ret;
    GlowFieldFlags fields;
    const GlowParameter *pParameter;
    const GlowMatrix *pMatrix;
    const GlowFunction *pFunction;
    int index;

    if(pThis->type == GlowElementType_Parameter) {
        pParameter = &pThis->glow.param;
        //qDebug("P %04d %s\n", pThis->number, pParameter->pIdentifier);
        ret.append(QString("  |-- type: ") + QString("Parameter"));
        ret.append(QString("  |-- number: ") + QString::asprintf("%d",pThis->number));
        ret.append(QString("  |-- identifier: ") + QString(pParameter->pIdentifier));

        fields = pThis->paramFields;

        if(fields & GlowFieldFlag_Description) {
            // qDebug("  description:      %s\n", pParameter->pDescription);
            ret.append(QString("  |-- description: ") + QString(pParameter->pDescription));
        }

        if(fields & GlowFieldFlag_Value) {
            //qDebug("  value:            ");
            ret.append(QString("  |-- value: ") + printValue(&pParameter->value));
        }

        if(fields & GlowFieldFlag_Minimum) {
            //qDebug("  minimum:          ");
            ret.append(QString("  |-- minimum: ") + printMinMax(&pParameter->minimum));
        }

        if(fields & GlowFieldFlag_Maximum) {
            //qDebug("  maximum:          ");
            ret.append(QString("  |-- maximum: ") + printMinMax(&pParameter->maximum));
        }

        if(fields & GlowFieldFlag_Access)
            ret.append(QString("  |-- access: ") + printAccess(pParameter->access));
        //qDebug("  access:           %d\n", pParameter->access);
        if(fields & GlowFieldFlag_Factor)
            ret.append(QString("  |-- factor: ") + QString::asprintf("%d",pParameter->factor));
        //qDebug("  factor:           %d\n", pParameter->factor);
        if(fields & GlowFieldFlag_IsOnline)
            ret.append(QString("  |-- isOnline: ") + QString(pParameter->isOnline ? "true" : "false"));
        //qDebug("  isOnline:         %d\n", pParameter->isOnline);
        if(fields & GlowFieldFlag_Step)
            ret.append(QString("  |-- step: ") + QString::asprintf("%d",pParameter->step));
        //qDebug("  step:             %d\n", pParameter->step);
        if(fields & GlowFieldFlag_Type)
            ret.append(QString("  |-- type: ") + QString::asprintf("%d",pParameter->type));
        //qDebug("  type:             %d\n", pParameter->type);
        if(fields & GlowFieldFlag_StreamIdentifier)
            ret.append(QString("  |-- streamIdentifier: ") + QString::asprintf("%d",pParameter->streamIdentifier));
        //qDebug("  streamIdentifier: %d\n", pParameter->streamIdentifier);

        if(fields & GlowFieldFlag_StreamDescriptor) {
            ret.append(QString("  |-- streamDescriptor: "));
            ret.append(QString("  |   |-- format: ") + QString::asprintf("%d",pParameter->streamDescriptor.format));
            ret.append(QString("  |   `-- offset: ") + QString::asprintf("%d",pParameter->streamDescriptor.offset));
            //qDebug("  streamDescriptor:\n");
            //qDebug("    format:         %d\n", pParameter->streamDescriptor.format);
            //qDebug("    offset:         %d\n", pParameter->streamDescriptor.offset);
        }

        if(pParameter->pEnumeration != NULL) {
            ret.append(QString("  |-- enumeration: "));
            ret.append(printEnum(pParameter));
            //qDebug("  enumeration:\n%s\n", pParameter->pEnumeration);
        }
        if(pParameter->pFormat != NULL)
            ret.append(QString("  |-- format: ") + QString(pParameter->pFormat));
        //qDebug("  format:           %s\n", pParameter->pFormat);
        if(pParameter->pFormula != NULL)
            ret.append(QString("  |-- formula: ") + QString(pParameter->pFormula));
        //qDebug("  formula:\n%s\n", pParameter->pFormula);
        if(pParameter->pSchemaIdentifiers != NULL)
            ret.append(QString("  |-- schemaIdentifiers: ") + QString(pParameter->pSchemaIdentifiers));
        //qDebug("  schemaIdentifiers: %s\n", pParameter->pSchemaIdentifiers);

    } else if(pThis->type == GlowElementType_Node) {
        //qDebug("N %04d %s\n", pThis->number, pThis->glow.node.pIdentifier);
        ret.append(QString("  |-- type: ") + QString("Node"));
        ret.append(QString("  |-- number: ") + QString::asprintf("%d",pThis->number));
        ret.append(QString("  |-- identifier: ") + QString(pThis->glow.node.pIdentifier));

        ret.append(QString("  |-- description: ") + QString(pThis->glow.node.pDescription));
        ret.append(QString("  |-- isRoot: ") + QString(pThis->glow.node.isRoot ? "true" : "false"));
        ret.append(QString("  |-- isOnline: ") + QString(pThis->glow.node.isOnline ? "true" : "false"));
        //qDebug("  description:      %s\n", pThis->glow.node.pDescription);
        //qDebug("  isRoot:           %s\n", pThis->glow.node.isRoot ? "true" : "false");
        //qDebug("  isOnline:         %s\n", pThis->glow.node.isOnline ? "true" : "false");

        if(pThis->glow.node.pSchemaIdentifiers != NULL)
            ret.append(QString("  |-- schemaIdentifiers: ") + QString(pThis->glow.node.pSchemaIdentifiers));
        //qDebug("  schemaIdentifiers: %s\n", pThis->glow.node.pSchemaIdentifiers);

    } else if(pThis->type == GlowElementType_Matrix) {
        pMatrix = &pThis->glow.matrix.matrix;
        //qDebug("M %04d %s\n", pThis->number, pMatrix->pIdentifier);
        ret.append(QString("  |-- type: ") + QString("Matrix"));
        ret.append(QString("  |-- number: ") + QString::asprintf("%d",pThis->number));
        ret.append(QString("  |-- identifier: ") + QString(pMatrix->pIdentifier));

        if(pMatrix->pDescription != NULL)
            ret.append(QString("  |-- description: ") + QString(pMatrix->pDescription));
        //qDebug("  description:                 %s\n", pMatrix->pDescription);
        if(pMatrix->type != GlowMatrixType_OneToN)
            ret.append(QString("  |-- type: ") + QString::asprintf("%d",pMatrix->type));
        //qDebug("  type:                        %d\n", pMatrix->type);
        if(pMatrix->addressingMode != GlowMatrixAddressingMode_Linear)
            ret.append(QString("  |-- addressingMode: ") + QString::asprintf("%d",pMatrix->addressingMode));
        //qDebug("  addressingMode:              %d\n", pMatrix->addressingMode);
        ret.append(QString("  |-- targetCount: ") + QString::asprintf("%d",pMatrix->targetCount));
        ret.append(QString("  |-- sourceCount: ") + QString::asprintf("%d",pMatrix->sourceCount));
        //qDebug("  targetCount:                 %d\n", pMatrix->targetCount);
        //qDebug("  sourceCount:                 %d\n", pMatrix->sourceCount);
        if(pMatrix->maximumTotalConnects != 0)
            ret.append(QString("  |-- maximumTotalConnects: ") + QString::asprintf("%d",pMatrix->maximumTotalConnects));
        //qDebug("  maximumTotalConnects:        %d\n", pMatrix->maximumTotalConnects);
        if(pMatrix->maximumConnectsPerTarget != 0)
            ret.append(QString("  |-- maximumConnectsPerTarget: ") + QString::asprintf("%d",pMatrix->maximumConnectsPerTarget));
        //qDebug("  maximumConnectsPerTarget:    %d\n", pMatrix->maximumConnectsPerTarget);
        if(glowParametersLocation_isValid(&pMatrix->parametersLocation)) {
            //qDebug("  parametersLocation:          ");
            QString id;
            if(pMatrix->parametersLocation.kind == GlowParametersLocationKind_BasePath) {
                for(index = 0; index < pMatrix->parametersLocation.choice.basePath.length; index++) {
                    id.append(QString::asprintf("%d", pMatrix->parametersLocation.choice.basePath.ids[index]));
                    //qDebug("%d", pMatrix->parametersLocation.choice.basePath.ids[index]);
                    if(index < pMatrix->parametersLocation.choice.basePath.length - 1)
                        id.append(".");
                    //qDebug(".");
                }
                //qDebug("\n");
            } else if(pMatrix->parametersLocation.kind == GlowParametersLocationKind_Inline) {
                id.append(QString::asprintf("%d",pMatrix->parametersLocation.choice.inlineId));
                //qDebug("%d\n", pMatrix->parametersLocation.choice.inlineId);
            }
            ret.append(QString("  |-- parametersLocation: ") + id);
        }
        if(pMatrix->pSchemaIdentifiers != NULL)
            ret.append(QString("  |-- schemaIdentifiers: ") + QString(pMatrix->pSchemaIdentifiers));
        //qDebug("  schemaIdentifiers:            %s\n", pMatrix->pSchemaIdentifiers);

    } else if(pThis->type == GlowElementType_Function) {
        pFunction = &pThis->glow.function;
        //qDebug("F %04d %s\n", pThis->number, pFunction->pIdentifier);
        ret.append(QString("  |-- type: ") + QString("Function"));
        ret.append(QString("  |-- number: ") + QString::asprintf("%d",pThis->number));
        ret.append(QString("  |-- identifier: ") + QString(pFunction->pIdentifier));

        if(pFunction->pDescription != NULL)
            ret.append(QString("  |-- description: ") + QString(pFunction->pDescription));
        //qDebug("  description:                 %s\n", pFunction->pDescription);
        if(pFunction->pArguments != NULL) {
            //qDebug("  arguments:\n");
            ret.append(QString("  |-- arguments: "));
            for(index = 0; index < pFunction->argumentsLength; index++)
                ret.append(QString("  |   |-- %1: %2:%3").arg(index).arg(pFunction->pArguments[index].pName).arg(pFunction->pArguments[index].type));
            //qDebug("    %s:%d\n", pFunction->pArguments[index].pName, pFunction->pArguments[index].type);
        }
        if(pFunction->pResult != NULL) {
            //qDebug("  result:\n");
            ret.append(QString("  |-- arguments: "));
            for(index = 0; index < pFunction->resultLength; index++)
                ret.append(QString("  |   |-- %1: %2:%3").arg(index).arg(pFunction->pResult[index].pName).arg(pFunction->pResult[index].type));
            //qDebug("    %s:%d\n", pFunction->pResult[index].pName, pFunction->pResult[index].type);
        }
    }
    return ret;
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
    pSession->obj->nodeReturned(pElement, pPath, pathLength);
}

void libember_slim_wrapper::onParameter(const GlowParameter *pParameter, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state)
{
   Session *pSession = (Session *)state;
   Element *pElement;
   Element *pParent;
   GlowParameter *pLocalParam;
    //qDebug() << "recieved Parameter";
   // if received element is a child of current cursor, print it
//   if(memcmp(pPath, pSession->pCursorPath, pSession->cursorPathLength * sizeof(berint)) == 0
//   && pathLength == pSession->cursorPathLength + 1)
//      qDebug("* P %04d %s\n", pPath[pathLength - 1], pParameter->pIdentifier);

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
//      if(pElement == pSession->pCursor
//      && (fields & GlowFieldFlag_Value))
//      {
//         printValue(&pParameter->value);
//         qDebug("\n");
//      }
      pSession->obj->parameterReturned(pElement, pPath, pathLength);
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

      pSession->obj->parameterReturned(pElement, pPath, pathLength);
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
    //qDebug() << "recieved UnsupportedTltlv";
   if(position == GlowReaderPosition_ParameterContents)
   {
      if(berTag_equals(&pReader->tag, &glowTags.parameterContents.enumeration))
      {
         pSession->pEnumeration = newarr(char, pReader->length + 1);
         berReader_getString(pReader, pSession->pEnumeration, pReader->length);
         //qDebug() << "recieved Enum: " << pSession->pEnumeration;
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
    // qDebug() << "recieved LastPackage";
}


libember_slim_wrapper::libember_slim_wrapper(QObject *parent) : QObject(parent)
{
    ember_init(onThrowError, onFailAssertion, allocMemoryImpl, freeMemoryImpl);
    m_tcpSock = new QTcpSocket(this);
    m_tcpSock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_tcpSock, &QIODevice::readyRead, this, &libember_slim_wrapper::readSocket);
    connect(m_tcpSock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &libember_slim_wrapper::socketError);
    m_readingTimeOut = new QTimer(this);
    connect(m_readingTimeOut, &QTimer::timeout, this, &libember_slim_wrapper::walkFinished);
    m_readingTimeOut->setSingleShot(true);
    m_socketTimeOut = new QTimer(this);
    connect(m_socketTimeOut, &QTimer::timeout, this, &libember_slim_wrapper::socketTimeOut);
    m_socketTimeOut->setSingleShot(true);
}

libember_slim_wrapper::~libember_slim_wrapper()
{
    qDebug() << "Destruct libember_slim_wrapper";
    if(m_session.root.type == GlowElementType_Node){
        element_free(&m_session.root);
    }
    delete m_readingTimeOut;
    delete m_socketTimeOut;
    delete m_tcpSock;
    if(pRxBuffer == nullptr)
            delete pRxBuffer;
}

void libember_slim_wrapper::connectEmber(QUrl &url, int &timeOut)
{
    m_url = url;
    QString host = m_url.host();
    int port = m_url.port(EMBER_DEF_PORT);
    m_readingTimeOut->setInterval(timeOut);
    m_socketTimeOut->setInterval(timeOut * 2);

    bzero_item(m_session);
    m_session.obj = this;
    m_session.pCursor = &m_session.root;
    m_session.remoteAddress = host.toStdString().c_str();
    m_session.remotePort = port;
    element_init(&m_session.root, NULL, GlowElementType_Node, 0);

    connect(m_tcpSock, &QTcpSocket::connected, this, &libember_slim_wrapper::socketConnected);

    m_tcpSock->connectToHost(host, port);
    m_socketTimeOut->start();
}

void libember_slim_wrapper::walkTree(byte &flags, const QStringList &path, const QString &value)
{
    m_flags = flags;
    m_startPath = path;
    m_writeVal = value;
    m_written = false, m_found = false;
    m_output.clear();
    Element *next, *start = &m_session.root;
    if (!m_startPath.isEmpty()) {
        for (int i = 0; i < m_startPath.size(); i++) {
            if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                next = element_findChild(start, m_startPath.at(i).toInt());
            else
                next = element_findChildByIdentifier(start, m_startPath.at(i).toStdString().c_str());
            if (next) {
                start = next;
                callChild(start);
            } else {
                getDirectory(start);
                m_readingTimeOut->start();
                return;
            }
        }
        m_found = true;
        if (start->type == GlowElementType_Node) {
            getDirectory(start);
            m_readingTimeOut->start();
        } else {
            if (m_flags & EMBER_FLAGS_WRITE && !m_written) {
                m_written = setParameterValue(start, m_writeVal);
                m_readingTimeOut->start();
            } else {
                findParams(start);
                emit finishedWalk(m_output);
            }
        }
    } else {
        m_found = true;
        getDirectory(start);
        m_readingTimeOut->start();
    }
}

void libember_slim_wrapper::disconnect()
{
    m_tcpSock->disconnectFromHost();
}

void libember_slim_wrapper::send(QByteArray msg)
{
    if(m_tcpSock && m_tcpSock->isOpen() && m_tcpSock->isWritable()){
        m_tcpSock->write(msg);
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
    //getDirectory(m_session.pCursor);
}

void libember_slim_wrapper::nodeReturned(Element *pElement, const berint *pPath, int pathLength)
{
    Q_UNUSED(pPath);
    if((pathLength) > m_startPath.size()){
        getDirectory(pElement);
        m_readingTimeOut->start();
    } else {
        bool result;
        if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                result = (pElement->number == m_startPath.value(pathLength -1).toInt());
        else
                result = !strcmp(pElement->glow.node.pIdentifier, m_startPath.value(pathLength - 1).toStdString().c_str());

        if (result) {
            if(pathLength == m_startPath.size())
                    m_found = true;
            callChild(pElement);
            getDirectory(pElement);
            m_readingTimeOut->start();
        }
    }
}

void libember_slim_wrapper::parameterReturned(Element *pElement, const berint *pPath, int pathLength)
{
    Q_UNUSED(pPath);
    if(pathLength == m_startPath.size() && pElement->type == GlowElementType_Parameter){
        bool result;
        if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                result = (pElement->number == m_startPath.value(pathLength -1).toInt());
        else
                result = !strcmp(pElement->glow.node.pIdentifier, m_startPath.value(pathLength - 1).toStdString().c_str());

        if (result) {                   // Parameter Found
            m_readingTimeOut->stop();
            m_found = true;
            callChild(pElement);
            if (m_flags & EMBER_FLAGS_WRITE && !m_written) {
                m_written = setParameterValue(pElement, m_writeVal);
                m_readingTimeOut->start();
            } else {
                findParams(pElement);
                emit finishedWalk(m_output);
            }
        }
    }
}

void libember_slim_wrapper::findParams(Element *pStart)
{
    Element *pThis = pStart;
    if(pThis != NULL) {
        if(pThis->type == GlowElementType_Parameter){           // <------ TODO: add other types for PrintOut
            printParam(pThis);
        } else {
            for(int i = 1; i <= pThis->children.count ; i++){
                Element *child = element_findChild(pThis, i);
                if(child != NULL)
                        findParams(child);
            }
        }
    }
}

void libember_slim_wrapper::printParam(Element *param)
{
    QString numPath, identPath, val;
    int pathLength = GLOW_MAX_TREE_DEPTH;
    berint pathBuffer[GLOW_MAX_TREE_DEPTH];
    berint *path = element_getPath(param, pathBuffer, &pathLength);
    for(int i = 0; i < pathLength; i++){
        numPath.append(QString::number(path[i])+".");
    }
    QString numPathFixed = numPath.left(numPath.lastIndexOf(QChar('.'))).append("/:");
    char identPathBuffer[IDENT_PATH_BUFFER];
    identPath = element_getIdentifierPath(param, identPathBuffer, IDENT_PATH_BUFFER);
    identPath.append("/:");
    val = returnValue(&param->glow.param.value);
    qDebug() <<  numPathFixed << " | " << identPath << val;
    if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                m_output.append(numPathFixed+val);
    else
                m_output.append(identPath+val);
    if (m_flags & EMBER_FLAGS_VERBOSE_OUT)
                m_output.append(element_print(param));

}

bool libember_slim_wrapper::setParameterValue(Element *pElement, QString &valueString)
{
    if (pElement->type != GlowElementType_Parameter){
        return false;
    }
    int pathLength = GLOW_MAX_TREE_DEPTH;
    berint pathBuffer[GLOW_MAX_TREE_DEPTH];
    berint *path = element_getPath(pElement, pathBuffer, &pathLength);
    pcstr pValueString = newarr(char, valueString.toStdString().length() +1);
    pValueString = valueString.toStdString().c_str();
    GlowOutput output;
    const int bufferSize = 512;
    byte *pBuffer;
    GlowParameterType type = element_getParameterType(pElement);
    GlowParameter parameter;

    bzero_item(parameter);

    switch(type)
    {
        case GlowParameterType_Integer:
        case GlowParameterType_Enum:
            sscanf(pValueString, "%lld", &parameter.value.choice.integer);
            parameter.value.flag = GlowParameterType_Integer;
            break;

        case GlowParameterType_Boolean:
            parameter.value.choice.boolean = atoi(pValueString) != 0;
            parameter.value.flag = GlowParameterType_Boolean;
            break;

        case GlowParameterType_Real:
            sscanf(pValueString, "%lf", &parameter.value.choice.real);
            parameter.value.flag = GlowParameterType_Real;
            break;

        case GlowParameterType_String:
            parameter.value.choice.pString = (pstr)pValueString;
            parameter.value.flag = GlowParameterType_String;
            break;

        default:
            return false;
    }
    if(!checkParamInBounds(parameter, pElement)) {
        return false;
    }
    qDebug() << "Write to Element " << pElement->glow.param.pIdentifier << " Value: " << pValueString;

    pBuffer = newarr(byte, bufferSize);
    glowOutput_init(&output, pBuffer, bufferSize, 0);
    glowOutput_beginPackage(&output, true);
    glow_writeQualifiedParameter(&output, &parameter, GlowFieldFlag_Value, path, pathLength);
    send(QByteArray((char *)pBuffer, glowOutput_finishPackage(&output)));
    freeMemory(pBuffer);
    return true;
}

bool libember_slim_wrapper::checkParamInBounds(const GlowParameter &param, const Element *elem)
{
    const GlowFieldFlags &fields = elem->paramFields;
    if (fields & GlowFieldFlag_Minimum && fields & GlowFieldFlag_Maximum){
        const GlowMinMax &min = elem->glow.param.minimum;
        const GlowMinMax &max = elem->glow.param.maximum;
        QString completePath;
        if (m_flags & EMBER_FLAGS_NUMBER_OUT)
                completePath = QString(m_startPath.join(".")).append("/:");
        else
                completePath = QString(m_startPath.join("/")).append("/:");

        switch(elem->glow.param.value.flag)
        {
            case GlowParameterType_Integer:
                if (!(param.value.choice.integer > min.choice.integer && param.value.choice.integer < max.choice.integer)) {
                    emit error(126, QString("%1 Value (%2) is not inside minimum (%3) and maximum (%4). Not written.").arg(completePath).arg(param.value.choice.integer).arg(min.choice.integer).arg(max.choice.integer));
                    return false;
                }
                break;

            case GlowParameterType_Real:
                if (!(param.value.choice.real > min.choice.real && param.value.choice.real < max.choice.real)) {
                    emit error(126, QString("%1 Value (%2) is not inside minimum (%3) and maximum (%4). Not written.").arg(completePath).arg(param.value.choice.real).arg(min.choice.real).arg(max.choice.real));
                    return false;
                }
                break;

            default:
                break;
        }
        return true;
    } else
            return true;
}

void libember_slim_wrapper::readSocket()
{
    QByteArray newData = m_tcpSock->readAll();
    if(newData.isEmpty())
        return;
    byte *pNewData = (byte *) newData.data();
    //qDebug() << "reading from Socket: " << newData << " | " << newData.size();
    glowReader_readBytes(&m_reader, pNewData, newData.size());
}

void libember_slim_wrapper::socketConnected()
{
    qDebug() << "Connected to: " << m_url.toString();
    m_socketTimeOut->stop();
    const int rxBufferSize = 1290; // max size of unescaped package
    pRxBuffer = newarr(byte, rxBufferSize);
    glowReader_init(&m_reader, onNode, onParameter, NULL, NULL, (voidptr)&m_session, pRxBuffer, rxBufferSize);
    m_reader.base.onMatrix = onMatrix;
    m_reader.base.onTarget = onTarget;
    m_reader.base.onSource = onSource;
    m_reader.base.onConnection = onConnection;
    m_reader.base.onFunction = onFunction;
    m_reader.base.onInvocationResult = onInvocationResult;
    m_reader.onOtherPackageReceived = onOtherPackageReceived;
    m_reader.base.onUnsupportedTltlv = onUnsupportedTltlv;
    m_reader.onLastPackageReceived = onLastPackageRecieved;

    emit emberConnected();
}

void libember_slim_wrapper::socketTimeOut()
{
    m_tcpSock->disconnectFromHost();
    emit error(1, QString("Connection to %1/ timed out. Is the Host online?").arg(m_url.toString()));
}

void libember_slim_wrapper::socketError(QAbstractSocket::SocketError socketError)
{
    QString msg;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        msg.append("TCP-Socket Closed: " + m_tcpSock->errorString());
        break;
    case QAbstractSocket::HostNotFoundError:
        msg.append("TCP-Socket 404 Error: " + m_tcpSock->errorString());
        break;
    case QAbstractSocket::ConnectionRefusedError:
        msg.append("TCP-Socket Error: " + m_tcpSock->errorString());
        break;
    default:
        msg.append("TCP-Socket Error: " + m_tcpSock->errorString());
    }
    emit error(1, msg);
}

void libember_slim_wrapper::walkFinished()
{
    QString msg, completePath;
    if (m_flags & EMBER_FLAGS_NUMBER_OUT)
            completePath = QString(m_startPath.join(".")).append("/:");
    else
            completePath = QString(m_startPath.join("/")).append("/:");

    if (!m_found) {
        msg = QString("Ember+ Path: %1 not found!").arg(completePath);
        emit error(126, msg);
    } else if (!m_written && m_flags & EMBER_FLAGS_WRITE) {
        msg = QString("Ember+ Path: %1 found but could not be written!").arg(completePath);
        emit error(126, msg);
    } else {
        findParams(m_session.pCursor);
        emit finishedWalk(m_output);
    }
}

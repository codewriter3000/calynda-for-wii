/*
 * mii.h — libmii: Read Mii data from the Wii's NAND filesystem.
 *
 * Original library by libmii authors.  Provides loadMiis_Wii() to
 * read all 100 Mii slots from shared2/menu/FaceLib/RFL_DB.dat.
 */

#ifndef MII_H
#define MII_H

#define RNOD

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef RNOD
#define MII_NAME_LENGTH    10
#define MII_CREATOR_LENGTH 10
#define MII_SIZE           74
#define MII_MAX            100
#define MII_HEADER         4

#define RED         0
#define ORANGE      1
#define YELLOW      2
#define LIGHT_GREEN 3
#define GREEN       4
#define BLUE        5
#define LIGHT_BLUE  6
#define PINK        7
#define PURPLE      8
#define BROWN       9
#define WHITE       10
#define BLACK       11

typedef struct {
    int invalid;
    int female;
    int month;
    int day;
    int favColor;
    int favorite;
    char name[MII_NAME_LENGTH * 2 + 1];
    int height;
    int weight;
    int miiID1;
    int miiID2;
    int miiID3;
    int miiID4;
    int systemID0;
    int systemID1;
    int systemID2;
    int systemID3;
    int faceShape;
    int skinColor;
    int facialFeature;
    int downloaded;
    int hairType;
    int hairColor;
    int hairPart;
    int eyebrowType;
    int eyebrowRotation;
    int eyebrowColor;
    int eyebrowSize;
    int eyebrowVertPos;
    int eyebrowHorizSpacing;
    int eyeType;
    int eyeRotation;
    int eyeVertPos;
    int eyeColor;
    int eyeSize;
    int eyeHorizSpacing;
    int noseType;
    int noseSize;
    int noseVertPos;
    int lipType;
    int lipColor;
    int lipSize;
    int lipVertPos;
    int glassesType;
    int glassesColor;
    int glassesSize;
    int glassesVertPos;
    int mustacheType;
    int beardType;
    int facialHairColor;
    int mustacheSize;
    int mustacheVertPos;
    int mole;
    int moleSize;
    int moleVertPos;
    int moleHorizPos;
    char creator[MII_CREATOR_LENGTH * 2 + 1];
} Mii;
#endif

Mii *loadMiis_Wii(void);
Mii *loadMiis(char *data);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* MII_H */

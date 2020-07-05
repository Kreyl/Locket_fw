/*
 * JsonStrings.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#pragma once

const char* JsonCfg =
"{Reactions: \
    [{\
        Name: Ari, \
        Sequence: [{Color: [0, 255, 0]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}], \
        VibroType: BrrBrrBrr\
    },\
    {\
        Name: Kaesu, \
        Sequence: [{Color: [255, 0, 0]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}], \
        VibroType: BrrBrrBrr\
    },\
    {\
        Name: Silent, \
        Transmit: 0\
    }, \
    {\
        Name: North, \
        Sequence: [{Color: [255, 255, 0]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}], \
        VibroType: Brr\
    },\
    {\
        Name: NorthStrong, \
        Sequence: [{Color: [255, 255, 0]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}], \
        VibroType: Brr\
    },\
    {\
        Name: South, \
        Sequence: [{Color: [0, 255, 255]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}\
                     ],  \
        VibroType: Brr\
    },\
    {\
        Name: SouthStrong, \
        Sequence: [{Color: [0, 255, 255]}, {Wait: 1000}, \
                     {Color: Off}, {Wait: 1000}\
                     ],  \
        VibroType: Brr\
    },\
    {\
        Name: Cursed, \
        Sequence: [{Color: [255, 128, 0]}, {Wait: 1000}, \
                     {Color: [255, 0, 0]}, {Wait: 1000},\
                     {Color: Off}, {Wait: 1000}\
                     ],  \
        VibroType: Brr\
    }],\
    \
    \
Lockets:[{\
        Name: Ari, \
        Type: 1,\
        FirstButton: Silent\
    },\
    {\
        Name: Kaesu,\
        Type: 2,\
        FirstButton: Silent\
    }, \
    {\
        Name: North, \
        Type: 3,\
        Receive: \
            [{\
                Source: Ari,\
                Reaction: Ari,\
                Distance: 12\
            },\
            {\
                Source: Kaesu,\
                Reaction: Kaesu,\
                Distance: 12\
            },\
            {\
                Source: Silent,\
                Reaction: Silent,\
                Distance: 12\
            },\
            ]\
    },\
    {\
        Name: NorthStrong, \
        Type: 4,\
        Button: 0,\
        Receive: \
            [{\
                Source: Ari,\
                Reaction: Ari,\
                Distance: 12\
            },\
            {\
                Source: Kaesu,\
                Reaction: Kaesu,\
                Distance: 12\
            },\
            {\
                Source: Silent,\
                Reaction: Silent,\
                Distance: 12\
            },\
            {\
                Source: North,\
                Reaction: North,\
                Distance: 6\
            },  \
            {\
                Source: NorthStrong,\
                Reaction: NorthStrong,\
                Distance: 6\
            },  \
            {\
                Source: South,\
                Reaction: South,\
                Distance: 6\
            },  \
            {\
                Source: SouthStrong,\
                Reaction: SouthStrong,\
                Distance: 6\
            },          \
            {\
                Source: Cursed,\
                Reaction: Cursed,\
                Distance: 6\
            }\
            ]\
    },\
    {\
        Name: South, \
        Type: 5,\
        Receive: \
            [{\
                Source: Ari,\
                Reaction: Ari,\
                Distance: 6\
            },\
            {\
                Source: Kaesu,\
                Reaction: Kaesu,\
                Distance: 6\
            },\
            {\
                Source: Silent,\
                Reaction: Silent,\
                Distance: 12\
            }]\
    },\
    {\
        Name: SouthStrong,\
        Type: 6,\
        Button: 0,\
        Receive: \
            [{\
                Source: Ari,\
                Reaction: Ari,\
                Distance: 12\
            },\
            {\
                Source: Kaesu,\
                Reaction: Kaesu,\
                Distance: 12\
            },\
            {\
                Source: Silent,\
                Reaction: Silent,\
                Distance: 12\
            },\
            {\
                Source: North,\
                Reaction: North,\
                Distance: 6\
            },  \
            {\
                Source: NorthStrong,\
                Reaction: NorthStrong,\
                Distance: 6\
            },  \
            {\
                Source: South,\
                Reaction: South,\
                Distance: 6\
            },  \
            {\
                Source: SouthStrong,\
                Reaction: SouthStrong,\
                Distance: 6\
            },  \
            {\
                Source: Cursed,\
                Reaction: Cursed,\
                Distance: 6\
            }]\
    },  \
    {\
        Name: Cursed, \
        Type: 7,\
        Receive: \
            [{\
                Source: Ari,\
                Reaction: Ari,\
                Distance: 12\
            },\
            {\
                Source: Kaesu,\
                Reaction: Kaesu,\
                Distance: 12\
            },\
            {\
                Source: Silent,\
                Reaction: Silent,\
                Distance: 12\
            }]\
    }\
]}";

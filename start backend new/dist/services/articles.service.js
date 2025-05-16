"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getById = exports.getAll = void 0;
const getAll = async () => {
    /* fetch data here */
    return {
        statusCode: 200,
        body: [
            {
                title: 'Article title',
            }
        ]
    };
};
exports.getAll = getAll;
const getById = async (req) => {
    /* fetch data here */
    /* id: req.params?.id */
    return {
        statusCode: 200,
        body: {
            title: `Article title ${req.params?.id}`,
        }
    };
};
exports.getById = getById;

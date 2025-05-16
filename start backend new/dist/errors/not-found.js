"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.NotFoundError = void 0;
const StatusCode_1 = require("../enum/StatusCode");
const custom_api_1 = require("./custom-api");
class NotFoundError extends custom_api_1.CustomAPIError {
    statusCode;
    constructor(message) {
        super(message);
        this.statusCode = StatusCode_1.HttpStatusCodes.NOT_FOUND;
    }
}
exports.NotFoundError = NotFoundError;

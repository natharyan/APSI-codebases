"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.server = exports.default = void 0;
const express_1 = __importDefault(require("express"));
const cors_1 = __importDefault(require("cors"));
const helmet_1 = __importDefault(require("helmet"));
const middleware = __importStar(require("./middleware"));
const articles_router_1 = __importDefault(require("./routers/articles.router"));
const auth_router_1 = __importDefault(require("./routers/auth.router"));
const connect_1 = require("./db/connect");
const dotenv_1 = __importDefault(require("dotenv"));
dotenv_1.default.config();
const ENV = process.env.NODE_ENV || 'production';
const PORT = process.env.PORT || 5500;
const MONGO_URI = process.env.MONGO_URI || '';
const app = (0, express_1.default)();
exports.default = app;
app.use((0, helmet_1.default)());
app.use((0, cors_1.default)());
app.use(express_1.default.json());
app.use(middleware.httpLogger);
app.get('/api/v1', (req, res) => {
    console.log('Home GET Request');
    res.status(200).json({ ms: 'Home GET Request', port: `${PORT}` });
});
app.use('/api/v1/articles', articles_router_1.default);
app.use('/api/v1/auth', auth_router_1.default);
app.use(middleware.errorHandler);
app.use(middleware.notFoundHandler);
const server = async () => {
    try {
        await (0, connect_1.connectDB)(MONGO_URI);
        app.listen(PORT, () => console.log(`Server is listening on PORT ${PORT}... on ${ENV} environment`));
    }
    catch (error) {
        console.log(error);
    }
};
exports.server = server;
server();

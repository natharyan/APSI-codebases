"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.default = void 0;
const express_1 = __importDefault(require("express"));
const auth_controller_1 = require("../controllers/auth.controller");
const router = express_1.default.Router();
exports.default = router;
router.post('/register', auth_controller_1.registerUser);
router.post('/login', auth_controller_1.loginUser);
router.delete('/logout', auth_controller_1.logoutUser);
router.post('/verify-email', auth_controller_1.verifyEmail);
router.post('/reset-password', auth_controller_1.resetPassword);
router.post('/forgot-password', auth_controller_1.forgotPassword);
router.post('/user-profile', auth_controller_1.userObj);

import express from 'express'
import { loginUser, registerUser, logoutUser, verifyEmail, resetPassword, forgotPassword, userObj } from '../controllers/auth.controller'

const router = express.Router()

router.post('/register', registerUser)
router.post('/login', loginUser)
router.delete('/logout', logoutUser)
router.post('/verify-email', verifyEmail)
router.post('/reset-password', resetPassword)
router.post('/forgot-password', forgotPassword)
router.post('/user-profile', userObj)

export { router as default }
